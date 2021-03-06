/*
 *   Copyright (C) 2010 by Scott Lawson KI4LKF
 *   Copyright (C) 2018 by Thomas A. Early N7TAE
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <time.h>
#include <libconfig.h++>
#include <string>

#include "QnetTypeDefs.h"
#include "Random.h"

using namespace libconfig;

#define VERSION "v1.0"

int sockDst = -1;
struct sockaddr_in toDst;

time_t tNow = 0;
short streamid_raw = 0;
bool isdefined[3] = { false, false, false };
std::string REPEATER, IP_ADDRESS;
int PORT, PLAY_WAIT, PLAY_DELAY;
bool is_icom = false;

unsigned char silence[9] = { 0x9E, 0x8D, 0x32, 0x88, 0x26, 0x1A, 0x3F, 0x61, 0xE8 };


unsigned short crc_tabccitt[256] = {
	0x0000,0x1189,0x2312,0x329b,0x4624,0x57ad,0x6536,0x74bf,0x8c48,0x9dc1,0xaf5a,0xbed3,0xca6c,0xdbe5,0xe97e,0xf8f7,
	0x1081,0x0108,0x3393,0x221a,0x56a5,0x472c,0x75b7,0x643e,0x9cc9,0x8d40,0xbfdb,0xae52,0xdaed,0xcb64,0xf9ff,0xe876,
	0x2102,0x308b,0x0210,0x1399,0x6726,0x76af,0x4434,0x55bd,0xad4a,0xbcc3,0x8e58,0x9fd1,0xeb6e,0xfae7,0xc87c,0xd9f5,
	0x3183,0x200a,0x1291,0x0318,0x77a7,0x662e,0x54b5,0x453c,0xbdcb,0xac42,0x9ed9,0x8f50,0xfbef,0xea66,0xd8fd,0xc974,
	0x4204,0x538d,0x6116,0x709f,0x0420,0x15a9,0x2732,0x36bb,0xce4c,0xdfc5,0xed5e,0xfcd7,0x8868,0x99e1,0xab7a,0xbaf3,
	0x5285,0x430c,0x7197,0x601e,0x14a1,0x0528,0x37b3,0x263a,0xdecd,0xcf44,0xfddf,0xec56,0x98e9,0x8960,0xbbfb,0xaa72,
	0x6306,0x728f,0x4014,0x519d,0x2522,0x34ab,0x0630,0x17b9,0xef4e,0xfec7,0xcc5c,0xddd5,0xa96a,0xb8e3,0x8a78,0x9bf1,
	0x7387,0x620e,0x5095,0x411c,0x35a3,0x242a,0x16b1,0x0738,0xffcf,0xee46,0xdcdd,0xcd54,0xb9eb,0xa862,0x9af9,0x8b70,
	0x8408,0x9581,0xa71a,0xb693,0xc22c,0xd3a5,0xe13e,0xf0b7,0x0840,0x19c9,0x2b52,0x3adb,0x4e64,0x5fed,0x6d76,0x7cff,
	0x9489,0x8500,0xb79b,0xa612,0xd2ad,0xc324,0xf1bf,0xe036,0x18c1,0x0948,0x3bd3,0x2a5a,0x5ee5,0x4f6c,0x7df7,0x6c7e,
	0xa50a,0xb483,0x8618,0x9791,0xe32e,0xf2a7,0xc03c,0xd1b5,0x2942,0x38cb,0x0a50,0x1bd9,0x6f66,0x7eef,0x4c74,0x5dfd,
	0xb58b,0xa402,0x9699,0x8710,0xf3af,0xe226,0xd0bd,0xc134,0x39c3,0x284a,0x1ad1,0x0b58,0x7fe7,0x6e6e,0x5cf5,0x4d7c,
	0xc60c,0xd785,0xe51e,0xf497,0x8028,0x91a1,0xa33a,0xb2b3,0x4a44,0x5bcd,0x6956,0x78df,0x0c60,0x1de9,0x2f72,0x3efb,
	0xd68d,0xc704,0xf59f,0xe416,0x90a9,0x8120,0xb3bb,0xa232,0x5ac5,0x4b4c,0x79d7,0x685e,0x1ce1,0x0d68,0x3ff3,0x2e7a,
	0xe70e,0xf687,0xc41c,0xd595,0xa12a,0xb0a3,0x8238,0x93b1,0x6b46,0x7acf,0x4854,0x59dd,0x2d62,0x3ceb,0x0e70,0x1ff9,
	0xf78f,0xe606,0xd49d,0xc514,0xb1ab,0xa022,0x92b9,0x8330,0x7bc7,0x6a4e,0x58d5,0x495c,0x3de3,0x2c6a,0x1ef1,0x0f78
};

void calcPFCS(unsigned char rawbytes[58])
{

	unsigned short crc_dstar_ffff = 0xffff;
	unsigned short tmp, short_c;
	short int i;

	for (i = 17; i < 56 ; i++) {
		short_c = 0x00ff & (unsigned short)rawbytes[i];
		tmp = (crc_dstar_ffff & 0x00ff) ^ short_c;
		crc_dstar_ffff = (crc_dstar_ffff >> 8) ^ crc_tabccitt[tmp];
	}
	crc_dstar_ffff =  ~crc_dstar_ffff;
	tmp = crc_dstar_ffff;

	rawbytes[56] = (unsigned char)(crc_dstar_ffff & 0xff);
	rawbytes[57] = (unsigned char)((tmp >> 8) & 0xff);
	return;
}

bool dst_open(const char *ip, const int port)
{
	int reuse = 1;

	sockDst = socket(PF_INET,SOCK_DGRAM,0);
	if (sockDst == -1) {
		printf("Failed to create DSTAR socket\n");
		return true;
	}
	if (setsockopt(sockDst,SOL_SOCKET,SO_REUSEADDR, (char *)&reuse, sizeof(reuse)) == -1) {
		close(sockDst);
		sockDst = -1;
		printf("setsockopt DSTAR REUSE failed\n");
		return true;
	}
	memset(&toDst,0,sizeof(struct sockaddr_in));
	toDst.sin_family = AF_INET;
	toDst.sin_port = htons(port);
	toDst.sin_addr.s_addr = inet_addr(ip);

	fcntl(sockDst,F_SETFL,O_NONBLOCK);
	return false;
}

void dst_close()
{
	if (sockDst != -1) {
		close(sockDst);
		sockDst = -1;
	}
	return;
}

bool get_value(const Config &cfg, const char *path, int &value, int min, int max, int default_value)
{
	if (cfg.lookupValue(path, value)) {
		if (value < min || value > max)
			value = default_value;
	} else
		value = default_value;
	printf("%s = [%d]\n", path, value);
	return true;
}

bool get_value(const Config &cfg, const char *path, double &value, double min, double max, double default_value)
{
	if (cfg.lookupValue(path, value)) {
		if (value < min || value > max)
			value = default_value;
	} else
		value = default_value;
	printf("%s = [%lg]\n", path, value);
	return true;
}

bool get_value(const Config &cfg, const char *path, bool &value, bool default_value)
{
	if (! cfg.lookupValue(path, value))
		value = default_value;
	printf("%s = [%s]\n", path, value ? "true" : "false");
	return true;
}

bool get_value(const Config &cfg, const char *path, std::string &value, int min, int max, const char *default_value)
{
	if (cfg.lookupValue(path, value)) {
		int l = value.length();
		if (l<min || l>max) {
			printf("%s is invalid\n", path);
			return false;
		}
	} else
		value = default_value;
	printf("%s = [%s]\n", path, value.c_str());
	return true;
}

/* process configuration file */
bool read_config(const char *cfgFile)
{
	Config cfg;

	printf("Reading file %s\n", cfgFile);
	// Read the file. If there is an error, report it and exit.
	try {
		cfg.readFile(cfgFile);
	} catch(const FileIOException &fioex) {
		printf("Can't read %s\n", cfgFile);
		return true;
	} catch(const ParseException &pex) {
		printf("Parse error at %s:%d - %s\n", pex.getFile(), pex.getLine(), pex.getError());
		return true;
	}

	if (! get_value(cfg, "ircddb.login", REPEATER, 3, 6, "UNDEFINED"))
		return true;
	REPEATER.resize(6, ' ');
	printf("REPEATER=[%s]\n", REPEATER.c_str());

	for (short int m=0; m<3; m++) {
		std::string path = "module.";
		path += m + 'a';
		std::string type;
		if (cfg.lookupValue(std::string(path+".type").c_str(), type)) {
			if (type.compare("dvap") && type.compare("dvrptr") && type.compare("mmdvm") && type.compare("icom") && type.compare("itap")) {
				printf("module type '%s' is invalid\n", type.c_str());
				return true;
			}
			is_icom = type.compare("icom") ? false : true;
			isdefined[m] = true;
		}
	}
	if (false==isdefined[0] && false==isdefined[1] && false==isdefined[2]) {
		printf("No repeaters defined!\n");
		return true;
	}

	if (! get_value(cfg, "gateway.internal.ip", IP_ADDRESS, 7, 15, is_icom ? "172.16.0.20" : "127.0.0.1"))
		return true;

	get_value(cfg, "gateway.internal.port", PORT, 16000, 65535, is_icom ? 20000 : 19000);

	get_value(cfg, "timing.play.wait", PLAY_WAIT, 1, 10, 1);

	get_value(cfg, "timing.play.delay", PLAY_DELAY, 9, 25, 19);

	return false;
}

void ToUpper(std::string &str)
{
	for (unsigned int i=0; i<str.size(); i++)
		if (islower(str[i]))
			str[i] = toupper(str[i]);
}

int main(int argc, char *argv[])
{
	unsigned short G2_COUNTER = 0;

	if (argc != 4) {
		printf("Usage: %s <module> <mycall> <yourcall>\n", argv[0]);
		printf("Example: %s c n7tae xrf757al\n", argv[0]);
		printf("Where...\n");
		printf("        c is the local repeater module\n");
		printf("        n7tae is the value of mycall\n");
		printf("        xrf757al is the value of yourcall, in this case this is a Link command\n\n");
		return 0;
	}

	std::string cfgfile(CFG_DIR);
	cfgfile += "/qn.cfg";
	if (read_config(cfgfile.c_str()))
		return 1;

	if (REPEATER.size() > 6) {
		printf("repeaterCallsign can not be more than 6 characters, %s is invalid\n", REPEATER.c_str());
		return 1;
	}
	ToUpper(REPEATER);

	char module = argv[1][0];
	if (islower(module))
		module = toupper(module);
	if ((module != 'A') && (module != 'B') && (module != 'C')) {
		printf("module must be one of A B C\n");
		return 1;
	}

	if (strlen(argv[2]) > 8) {
		printf("MYCALL can not be more than 8 characters, %s is invalid\n", argv[2]);
		return 1;
	}
	std::string mycall(argv[2]);
	ToUpper(mycall);


	if (strlen(argv[3]) > 8) {
		printf("YOURCALL can not be more than 8 characters, %s is invalid\n", argv[3]);
		return 1;
	}
	std::string yourcall(argv[3]);
	ToUpper(yourcall);

	unsigned long int delay = PLAY_DELAY * 1000;
	sleep(PLAY_WAIT);

	std::string RADIO_ID("QnetRemote ");
	RADIO_ID.append(VERSION);
	RADIO_ID.resize(20, ' ');

	time(&tNow);
	CRandom Random;

	if (dst_open(IP_ADDRESS.c_str(), PORT))
		return 1;

	SDSTR pkt;
	memcpy(pkt.pkt_id,"DSTR", 4);
	pkt.counter = htons(G2_COUNTER);
	pkt.flag[0] = 0x73;
	pkt.flag[1] = 0x12;
	pkt.flag[2] = 0x00;
	pkt.remaining = 0x30;
	pkt.vpkt.icm_id = 0x20;
	pkt.vpkt.dst_rptr_id = 0x00;
	pkt.vpkt.snd_rptr_id = 0x01;
	if (module == 'A')
		pkt.vpkt.snd_term_id = 0x03;
	else if (module == 'B')
		pkt.vpkt.snd_term_id = 0x01;
	else if (module == 'C')
		pkt.vpkt.snd_term_id = 0x02;
	else
		pkt.vpkt.snd_term_id = 0x00;
	streamid_raw = Random.NewStreamID();
	pkt.vpkt.streamid = htons(streamid_raw);
	pkt.vpkt.ctrl = 0x80;
	pkt.vpkt.hdr.flag[0] = pkt.vpkt.hdr.flag[1] = pkt.vpkt.hdr.flag[2] = 0x00;

	REPEATER.resize(7, ' ');
	memcpy(pkt.vpkt.hdr.r2, std::string(REPEATER + 'G').c_str(), 8);
	memcpy(pkt.vpkt.hdr.r1, std::string(REPEATER + module).c_str(), 8);
	mycall.resize(8, ' ');
	memcpy(pkt.vpkt.hdr.my, mycall.c_str(), 8);
	memcpy(pkt.vpkt.hdr.nm, "QNET", 4);
	if (yourcall.size() < 3)
		yourcall = std::string(8-yourcall.size(), ' ') + yourcall;	// right justify 1 or 2 letter commands
	else
		yourcall.resize(8, ' ');
	memcpy(pkt.vpkt.hdr.ur, yourcall.c_str(), 8);

	calcPFCS(pkt.pkt_id);
	// send the header
	int sent = sendto(sockDst, pkt.pkt_id, 58, 0, (struct sockaddr *)&toDst, sizeof(toDst));
	if (sent != 58) {
		printf("%s: ERROR: Couldn't send header!\n", argv[0]);
		dst_close();
		return 1;
	}

	// prepare and send 10 voice packets
	pkt.remaining = 0x13;
	memcpy(pkt.vpkt.vasd.voice, silence, 9);

	for (int i=0; i<10; i++) {
		usleep(delay);

		/* start sending silence + text */
		pkt.counter = htons(++G2_COUNTER);
		pkt.vpkt.ctrl = i;

		switch (i) {
			case 0:	// sync voice frame
				pkt.vpkt.vasd.text[0] = 0x55;
				pkt.vpkt.vasd.text[1] = 0x2d;
				pkt.vpkt.vasd.text[2] = 0x16;
				break;
			case 1:
				pkt.vpkt.vasd.text[0] = '@' ^ 0x70;
				pkt.vpkt.vasd.text[1] = RADIO_ID[0] ^ 0x4f;
				pkt.vpkt.vasd.text[2] = RADIO_ID[1] ^ 0x93;
				break;
			case 2:
				pkt.vpkt.vasd.text[0] = RADIO_ID[2] ^ 0x70;
				pkt.vpkt.vasd.text[1] = RADIO_ID[3] ^ 0x4f;
				pkt.vpkt.vasd.text[2] = RADIO_ID[4] ^ 0x93;
				break;
			case 3:
				pkt.vpkt.vasd.text[0] = 'A' ^ 0x70;
				pkt.vpkt.vasd.text[1] = RADIO_ID[5] ^ 0x4f;
				pkt.vpkt.vasd.text[2] = RADIO_ID[6] ^ 0x93;
				break;
			case 4:
				pkt.vpkt.vasd.text[0] = RADIO_ID[7] ^ 0x70;
				pkt.vpkt.vasd.text[1] = RADIO_ID[8] ^ 0x4f;
				pkt.vpkt.vasd.text[2] = RADIO_ID[9] ^ 0x93;
				break;
			case 5:
				pkt.vpkt.vasd.text[0] = 'B' ^ 0x70;
				pkt.vpkt.vasd.text[1] = RADIO_ID[10] ^ 0x4f;
				pkt.vpkt.vasd.text[2] = RADIO_ID[11] ^ 0x93;
				break;
			case 6:
				pkt.vpkt.vasd.text[0] = RADIO_ID[12] ^ 0x70;
				pkt.vpkt.vasd.text[1] = RADIO_ID[13] ^ 0x4f;
				pkt.vpkt.vasd.text[2] = RADIO_ID[14] ^ 0x93;
				break;
			case 7:
				pkt.vpkt.vasd.text[0] = 'C' ^ 0x70;
				pkt.vpkt.vasd.text[1] = RADIO_ID[15] ^ 0x4f;
				pkt.vpkt.vasd.text[2] = RADIO_ID[16] ^ 0x93;
				break;
			case 8:
				pkt.vpkt.vasd.text[0] = RADIO_ID[17] ^ 0x70;
				pkt.vpkt.vasd.text[1] = RADIO_ID[18] ^ 0x4f;
				pkt.vpkt.vasd.text[2] = RADIO_ID[19] ^ 0x93;
				break;
			case 9:	// terminal voice packet
				pkt.vpkt.ctrl |= 0x40;
				pkt.vpkt.vasd.text[0] = 0x70;
				pkt.vpkt.vasd.text[1] = 0x4f;
				pkt.vpkt.vasd.text[2] = 0x93;
				break;
		}

		sent = sendto(sockDst,pkt.pkt_id, 29, 0, (struct sockaddr *)&toDst, sizeof(toDst));
		if (sent != 29) {
			printf("%s: ERROR: could not send voice packet %d\n", argv[0], i);
			dst_close();
			return 1;
		}
	}
	dst_close();
	return 0;
}
