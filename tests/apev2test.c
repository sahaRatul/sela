#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>

#include "wavutils.h"
#include "apev2.h"

int main(int argc,char **argv)
{
	if(argc < 3)
	{
		fprintf(stderr,"Usage : %s <input.wav> <output>\n",argv[0]);
		return -1;
	}

	FILE *infile = fopen(argv[1],"r");
	FILE *outfile = fopen(argv[2],"w+");

	int32_t sample_rate,read_size;
	int16_t channels;
	int16_t bits_per_sample;
	uint32_t data_size;
	wav_tags tags;

	//Matadata structures
	apev2_state state,read_state;
	apev2_item_list ape_list,ape_read_list;
	apev2_keys keys_inst;
	apev2_hdr_ftr header,read_header;

	//Init wav tags
	init_wav_tags(&tags);

	//Check for tags
	int32_t is_wav = check_wav_file
	(	
		infile,
		&sample_rate,
		&channels,
		&bits_per_sample,
		&data_size,
		&tags
	);

	//Error cases
	switch(is_wav)
	{
		case READ_STATUS_OK:
			fprintf(stderr,"WAV file detected.\n");
			break;
		case READ_STATUS_OK_WITH_META:
			fprintf(stderr,"WAV file detected.\n");
			break;
		case ERR_NO_RIFF_MARKER:
			fprintf(stderr,"RIFF header not found. Exiting......\n");
			fclose(infile);
			fclose(outfile);
			return -1;
		case ERR_NO_WAVE_MARKER:
			fprintf(stderr,"WAVE header not found. Exiting......\n");
			fclose(infile);
			fclose(outfile);
			return -1;
		case ERR_NO_FMT_MARKER:
			fprintf(stderr,"No Format chunk found. Exiting......\n");
			fclose(infile);
			fclose(outfile);
			return -1;
		case ERR_NOT_A_PCM_FILE:
			fprintf(stderr,"Not a PCM file. Exiting.....\n");
			fclose(infile);
			fclose(outfile);
			return -1;
		default:
			fprintf(stderr,"Some error occured. Exiting.......\n");
			return -1;
	}

	//If metadata found then continue
	if (is_wav == READ_STATUS_OK_WITH_META)
	{
		fprintf(stderr,"-------Wav tags-----\n");
		fprintf(stderr,"Title : %s\n",tags.title);
		fprintf(stderr,"Artist : %s\n",tags.artist);
		fprintf(stderr,"Album : %s\n",tags.album);
		fprintf(stderr,"Genre : %s\n",tags.genre);
		fprintf(stderr,"Year : %c%c%c%c\n",tags.year[0],tags.year[1],tags.year[2],tags.year[3]);
		fprintf(stderr,"Comment : %s\n",tags.comment);
	}
	else
	{
		fprintf(stderr,"No metadata found in wav files\n");
		fclose(infile);
		fclose(outfile);
		return -1;
	}

	//Init ape state
	assert(APE_STATE_INIT_SUCCESS == init_apev2(&state));
	assert(APE_STATE_INIT_SUCCESS == init_apev2(&read_state));

	//Init ape keys
	assert(APE_KEYS_INIT_SUCCESS == init_apev2_keys(&state,&keys_inst));
	assert(APE_KEYS_INIT_SUCCESS == init_apev2_keys(&read_state,&keys_inst));
	//Init ape header
	assert(APE_HEADER_INIT_SUCCESS == init_apev2_header(&state,&header));
	assert(APE_HEADER_INIT_SUCCESS == init_apev2_header(&read_state,&read_header));

	//Init item link list
	assert(APE_LIST_INIT_SUCCESS == init_apev2_item_list(&state,&ape_list));
	assert(APE_LIST_INIT_SUCCESS == init_apev2_item_list(&read_state,&ape_read_list));

	//Convert wav tags to apev2 tags
	assert(APE_CONV_SUCCESS == wav_to_apev2(&state,&tags,&ape_list,&keys_inst));

	//Finalize header
	assert(APE_FINALIZE_HEADER_SUCCESS == finalize_apev2_header(&state,&header,&ape_list));

	//Write ape tags to output file
	assert(APE_FILE_WRITE_SUCCESS == write_apev2_tags(&state,outfile,ftell(outfile),&header,&ape_list));

	//Seek to end
	fseek(outfile,0,SEEK_END);
	read_size = (int32_t)ftell(outfile);
	char *data = (char *)malloc(sizeof(char) * read_size);

	//Seek to beginning
	rewind(outfile);
	fread(data,sizeof(char),(size_t)(read_size - 1),outfile);

	//Read apev2 tags from output file
	assert(APE_READ_SUCCESS == read_apev2_tags(&read_state,data,read_size,&keys_inst,&read_header,&ape_read_list));

	fprintf(stderr,"\n----------Apev2 tags-----------\n");
	//Print back the apev2 tags
	print_apev2_tags(&read_state,&ape_read_list);

	//Destroy wav tags
	destroy_wav_tags(&tags);
	free_apev2_list(&read_state,&ape_read_list);
	free_apev2_list(&state,&ape_list);

	free(data);
	fclose(infile);
	fclose(outfile);
	
	return 0;
}