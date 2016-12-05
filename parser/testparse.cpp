#include <dirent.h>
#include <err.h>
#include <cstdio>
#include <cstring>
#include <cerrno>
#include <ctime>
#include <iostream>
#include <fstream>
#include <string>

#include "parser.h"

using namespace std;

bool debug=false;

int main(int argc, char **argv)
{
	fldformat frm;
	field message(&frm);
	char format_dir[]="./formats";
	char filename[sizeof(format_dir)+NAME_MAX+1];
	DIR *frmdir;
	struct dirent *de;
	int frmcounter=0;
	string msgbuf;
	string msgbuf2;
	size_t msglen=0;
	size_t msglen1=0;
	size_t msglen2=0;
	FILE *outfile;
	

	if(argc>2)
	{
		printf("Usage %s [filename]\nfilename is the file to read the message from. If omitted, read from standard input\n", argv[0]);
		return 1;
	}

	frmdir=opendir(format_dir);

	if(!frmdir)
		err(2, "Can't open %s", format_dir);

	while((de=readdir(frmdir)))
	{
		#ifdef _DIRENT_HAVE_D_TYPE
			if(de->d_type!=DT_LNK && de->d_type!=DT_REG && de->d_type!=DT_UNKNOWN)
				continue;
		#endif

		if(strcmp(de->d_name+strlen(de->d_name)-4, ".frm"))
			continue;

		sprintf(filename, "%s/%s", format_dir, de->d_name);

		if(debug)
			printf("Loading %s\n", filename);

		frm.load_format(filename);

		frmcounter++;
	}

	closedir(frmdir);

	if(frmcounter==0)
	{
		printf("Error: No formats loaded\n");
		return 3;
	}

	if(debug)
		printf("Info: Loaded %d formats\n", frmcounter);

	if(debug)
		frm.print_format();

	if(argc>1)
	{
		std::ifstream infile(argv[1]);
		if(!infile)
			err(4, "Cannot open file %s", argv[1]);

		if(debug)
			printf("Reading from %s\n", argv[1]);

		msgbuf.assign(std::istreambuf_iterator<char>(infile), std::istreambuf_iterator<char>());

		infile.close();
	}
	else
	{
		if(debug)
			printf("Reading from stdin\n");
		msgbuf.assign(std::istreambuf_iterator<char>(cin), std::istreambuf_iterator<char>());
	}

	msglen=msgbuf.length();

	msglen1=msglen;

	try
	{
		message.parse(msgbuf);
	}
	catch(const exception& e)
	{
		printf("Error: Unable to parse message: %s\n", e.what());

		sprintf(filename, "imessage%ld", time(NULL));
		outfile=fopen(filename, "w");
		for(msglen=0; msglen<msglen1; fputc(msgbuf[msglen++], outfile));
		fclose(outfile);
		if(debug)
			printf("%s file is written\n", filename);

		return 6;
	}

	if(debug)
		printf("%s parsed, length: %lu\n", message.get_description().c_str(), message.get_cached_blength());

	message.print_message();

	message.reset_altformat();

	if(debug)
		printf("Building %s, estimated length: %lu\n", message.get_description().c_str(), message.get_blength());

	try
	{
		msglen2=message.serialize(msgbuf2);
	}
	catch(const exception& e)
	{
		if(debug)
			printf("Error: Unable to build %s: %s\n", message.get_description().c_str(), e.what());

		sprintf(filename, "imessage%ld", time(NULL));
		outfile=fopen(filename, "w");
		for(msglen=0; msglen<msglen1; fputc(msgbuf[msglen++], outfile));
		fclose(outfile);
		if(debug)
			printf("%s file is written\n", filename);

		return 7;
	}

	if(debug)
		printf("%s built. Length: %lu/%lu\n", message.get_description().c_str(), msglen2, msgbuf2.length());

	if(msglen2!=msglen)
	{
		if(debug)
			printf("Warning: Total length mismatch (%lu != %lu)\n", msglen, msglen2);
	}
	else
		for(msglen=0; msglen<msglen2; msglen++)
			if(msgbuf[msglen]!=msgbuf2[msglen])
			{
				if(debug)
					printf("Warning: Messages don't match (starting from byte %lu)\n", msglen);
				break;
			}

	if(msglen2==msglen)
	{
		if(debug)
			printf("Rebuilt message matches original\n");
	}
	else
	{
		sprintf(filename, "imessage%ld", time(NULL));
		outfile=fopen(filename, "w");
		for(msglen=0; msglen<msglen1; fputc(msgbuf[msglen++], outfile));
		fclose(outfile);

		filename[0]='o';
		outfile=fopen(filename, "w");
		for(msglen=0; msglen<msglen2; fputc(msgbuf2[msglen++], outfile));
		fclose(outfile);
		if(debug)
			printf("i%s and o%s files are written\n", filename+1, filename+1);

		return 8;
	}

	return 0;
}

