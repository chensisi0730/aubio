#include <aubio.h>
#include "utils_tests.h"
#define FILE_INTERFACE 1
#define WAV_MEM_INTERFACE 1
#define PCM_MEM_INTERFACE 1





int main (int argc, char **argv)
{
  uint_t err = 0;
  aubio_result_t rest ;
  if (argc < 2) {
    PRINT_WRN("no arguments, running tests\n");
    //err = test_wrong_params();
    PRINT_MSG("usage: %s <source_path> [samplerate] [win_size] [hop_size]\n",
        argv[0]);
    //return err;
  }
  uint_t samplerate = 0;
  if ( argc >= 3 ) samplerate = atoi(argv[2]);
  uint_t win_size = 1024; // window size
  if ( argc >= 4 ) win_size = atoi(argv[3]);
  uint_t hop_size = win_size / 32;  //32:hop_size =32  4:hop_size =256
  if ( argc >= 5 ) hop_size = atoi(argv[4]);
  uint_t n_frames = 0, read = 0;

  char_t *source_path = argv[1];
  //source_path = "44100Hz_44100f_sine441_stereo.wav";
  //source_path =   "bounce.mp3.wav";
#if FILE_INTERFACE
  aubio_source_t * source = new_aubio_source(source_path, samplerate, hop_size);
  if (!source) { err = 1; goto beach; }

  if (samplerate == 0 ) samplerate = aubio_source_get_samplerate(source);



  // create tempo object
  aubio_tempo_t * o = new_aubio_tempo("default", win_size, hop_size, samplerate);
  if (!o) { err = 1; goto beach_tempo; }

  aubio_source_compact_do(source , o ,hop_size , &rest);
    free(rest.pConfidence);
    free(rest.pPosition);
  beach_tempo:
    del_aubio_source(source);
  beach:
    aubio_cleanup();


  // clean up memory
  del_aubio_tempo(o);
//////////////  WAV mem  接口 ///////////
#elif WAV_MEM_INTERFACE 
{
	FILE* fp = NULL;
    
    aubio_result_t rest ;
    uint_t nFileLen =0;
    source_path = "bounce.mp3.wav";
    fp = fopen(source_path ,"rb");
    if (fp == NULL)
    {       
        return 0;   
    }  
    fseek(fp,0,SEEK_END); //定位到文件末    
    if ((nFileLen = ftell(fp))<1)//文件长度 
    {      
        fclose(fp);    
        return 0;   
    }       
    unsigned char  * data = (unsigned char  *)malloc(sizeof(char)*(nFileLen+1)); 
    if (NULL == data)   
    {       

        fclose(fp);     
        return 0;   
    }   
    fseek(fp,0,SEEK_SET);
    fread(data, nFileLen, 1, fp);
    fclose(fp);


    aubio_beat_compact_wav_mem_do(data , nFileLen , samplerate, win_size , hop_size , &rest);
    free(data);
    free(rest.pConfidence);
    free(rest.pPosition);


}
#elif PCM_MEM_INTERFACE


//////////////  PCM mem  接口 ///////////
{ 
	FILE* fp = NULL;
    
    aubio_result_t rest ;
    uint_t nFileLen =0;
    source_path = "bounce.pcm";
    fp = fopen(source_path ,"rb");
    if (fp == NULL)
    {       
        return 0;   
    }  
    fseek(fp,0,SEEK_END); //定位到文件末    
    if ((nFileLen = ftell(fp))<1)//文件长度 
    {      
        fclose(fp);    
        return 0;   
    }       
    unsigned char  * data = (unsigned char  *)malloc(sizeof(char)*(nFileLen+1)); 
    if (NULL == data)   
    {       

        fclose(fp);     
        return 0;   
    }   
    fseek(fp,0,SEEK_SET);
    fread(data, nFileLen, 1, fp);
    fclose(fp);


    samplerate = 44100 ;
    aubio_beat_compact_pcm_mem_do(data , nFileLen, 2 , 16 , samplerate,   win_size , hop_size , &rest);

    free(data);
    free(rest.pConfidence);
    free(rest.pPosition);

}
#endif 


  return err;
}

int test_wrong_params(void)
{
  uint_t win_size = 1024;
  uint_t hop_size = 256;
  uint_t samplerate = 44100;
  aubio_tempo_t *t;
  fvec_t* in, *out;
  uint_t i;
  return 0;

  // test wrong method fails
  if (new_aubio_tempo("undefined", win_size, hop_size, samplerate)) return 1;

  // test hop > win fails
  if (new_aubio_tempo("default", hop_size, win_size, samplerate)) return 1;

  // test null hop_size fails
  if (new_aubio_tempo("default", win_size, 0, samplerate)) return 1;

  // test 1 buf_size fails
  if (new_aubio_tempo("default", 1, 1, samplerate)) return 1;

  // test null samplerate fails
  if (new_aubio_tempo("default", win_size, hop_size, 0)) return 1;

  // test short sizes workaround
  t = new_aubio_tempo("default", 2048, 2048, 500);
  if (!t) return 1;

  del_aubio_tempo(t);

  t = new_aubio_tempo("default", win_size, hop_size, samplerate);
  if (!t) return 1;

  in = new_fvec(hop_size);
  out = new_fvec(1);

  // up to step = (next_power_of_two(5.8 * samplerate / hop_size ) / 4 )
  for (i = 0; i < 256 + 1; i++)
  {
    aubio_tempo_do(t,in,out);
    PRINT_MSG("beat at %.3fms, %.3fs, frame %d, %.2f bpm "
        "with confidence %.2f, was tatum %d\n",
        aubio_tempo_get_last_ms(t), aubio_tempo_get_last_s(t),
        aubio_tempo_get_last(t), aubio_tempo_get_bpm(t),
        aubio_tempo_get_confidence(t), aubio_tempo_was_tatum(t));
  }

  del_aubio_tempo(t);
  del_fvec(in);
  del_fvec(out);

  return run_on_default_source(main);
}
