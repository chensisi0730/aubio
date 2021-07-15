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
    //source_path = "bounce.mp3.wav";
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

    aubio_source_wav_mem_t* source = new_aubio_source_wav_mem(data , nFileLen , samplerate, hop_size);
    if (!source) { err = 1; goto beach; }

    if (samplerate == 0 ) samplerate = aubio_source_get_samplerate_wav_mem(source);

    // create tempo object
    aubio_tempo_t * o = new_aubio_tempo("default", win_size, hop_size, samplerate);

    if (!o) { err = 1; goto beach_tempo; }
    
    aubio_source_compact_wav_mem_do(source , o , hop_size , &rest);    
    free(rest.pConfidence);
    free(rest.pPosition);

    // clean up memory
    del_aubio_tempo(o);
    
    beach_tempo:
      del_aubio_source((aubio_source_t *)source);
    beach:
      aubio_cleanup();

      return err;

}
#elif PCM_MEM_INTERFACE


//////////////  PCM mem  接口 ///////////
{ 
    n_frames = 0 ;
    read = 0;
    char_t *source_path = "bounce.mp3.wav";
    samplerate = 44100 ;
    aubio_source_wavread_t * p ;
    aubio_source_t * source_my = new_aubio_source(source_path, samplerate,  hop_size);
    p = (aubio_source_wavread_t *)(source_my->source);
    if( p->data_addr == NULL){       
        PRINT_MSG("file: %s , func :%s , line:%d\n",__FILE__ , __func__ , __LINE__);
        return ;
    }
    
    PRINT_MSG("file: %s , func :%s , line:%d ,p->data_addr = %x ,p->data_size=%d\n",__FILE__ , __func__ , __LINE__ ,
        p->data_addr, p->data_size);//9922500 frames, p->data_size=39690000,4倍的关系才是对的
    aubio_source_t * source_mem = new_aubio_source_pcm_mem(p->data_addr, p->data_size, 2 , 16 , samplerate,   hop_size );

    if (!source_mem) { err = 1; goto beach; }
    if (samplerate == 0 ) samplerate = aubio_source_get_samplerate_mem(source_mem);

    // create tempo object
    aubio_tempo_t * o = new_aubio_tempo("default", win_size, hop_size, samplerate);

    if (!o) { err = 1; goto beach_tempo_mem; }
    //PRINT_MSG("file: %s , func :%s , line:%d  hop_size :%d\n",__FILE__ , __func__ , __LINE__ ,hop_size);

    aubio_source_compact_pcm_mem_do(source_mem , o , hop_size , &rest);
    free(p->data_addr);
    free(rest.pConfidence);
    free(rest.pPosition);

   

    beach_tempo_mem:
    PRINT_ERR("read !!!!!!!!!!!!!!!\n");
      del_aubio_source(source_mem);
      free(p->data_addr);

    // clean up memory
    del_aubio_tempo(o);
    beach:
      aubio_cleanup();

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
