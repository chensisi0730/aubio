#include <aubio.h>
#include "utils_tests.h"
#define FILE_INTERFACE 0
#define ZIJIZAO_MEM 0

int test_wrong_params(void);
#pragma GCC push_options
#pragma GCC optimize (0)

int main (int argc, char **argv)
{
  uint_t err = 0;
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
  source_path = "44100Hz_44100f_sine441_stereo.wav";

#if FILE_INTERFACE
  aubio_source_t * source = new_aubio_source(source_path, samplerate,
      hop_size);
  if (!source) { err = 1; goto beach; }

  if (samplerate == 0 ) samplerate = aubio_source_get_samplerate(source);

  // create some vectors
  fvec_t * in = new_fvec (hop_size); // input audio buffer  一次处理一个hop_size的数据
  fvec_t * out = new_fvec (1); // output position

  // create tempo object
  aubio_tempo_t * o = new_aubio_tempo("default", win_size, hop_size,
      samplerate);

  if (!o) { err = 1; goto beach_tempo; }

  do {
    // put some fresh data in input vector
    aubio_source_do(source, in, &read); //read = 256 = hop_size 一次从文件句柄中取出一个hop_size数据
    // execute tempo
    aubio_tempo_do(o,in,out);
    // do something with the beats
    if (out->data[0] != 0) {//自己造的数据，是不会有节拍的。
      PRINT_MSG("beat at %.3fms, %.3fs, frame %d, %.2f bpm "
          "with confidence %.2f\n",
          aubio_tempo_get_last_ms(o), aubio_tempo_get_last_s(o),
          aubio_tempo_get_last(o), aubio_tempo_get_bpm(o),
          aubio_tempo_get_confidence(o));
    }
    n_frames += read;
  } while ( read == hop_size );

  PRINT_MSG("read %.2fs, %d frames at %dHz (%d blocks) from %s\n",
      n_frames * 1. / samplerate,
      n_frames, samplerate,
      n_frames / hop_size, source_path);

  // clean up memory
  del_aubio_tempo(o);
#else


//////////////  mem  接口 ///////////

{ 
    n_frames = 0 ;
    read = 0;

    #if ZIJIZAO_MEM //mem  自己构造的数据，无法获得节拍，
    int DATA_LEN = 44100*2*2 ;//44100*2*2 ; //32*2*2 ;
    unsigned char  * p = (unsigned char *)malloc(DATA_LEN);// 单通道  16BIT
    memset(p ,0x55, DATA_LEN );
    if(p == NULL){
        PRINT_MSG("file: %s , func :%s , line:%d , *p = 0x%x\n",__FILE__ , __func__ , __LINE__, *p);
        return ;
    }
    
    PRINT_MSG("file: %s , func :%s , line:%d , *p = 0x%x\n",__FILE__ , __func__ , __LINE__, *p);
    samplerate = 44100 ;
    aubio_source_t * source_mem = new_aubio_source_mem(p , DATA_LEN, samplerate, hop_size , 16);
    #else

    
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
    aubio_source_t * source_mem = new_aubio_source_mem(p->data_addr, p->data_size, samplerate,   hop_size ,16);
    #endif



    if (!source_mem) { err = 1; goto beach; }

    if (samplerate == 0 ) samplerate = aubio_source_get_samplerate_mem(source_mem);

    // create some vectors
    fvec_t * in_mem = new_fvec (hop_size); // input audio buffer
    fvec_t * out_mem = new_fvec (1); // output position

    // create tempo object
    aubio_tempo_t * o = new_aubio_tempo("default", win_size, hop_size, samplerate);

    if (!o) { err = 1; goto beach_tempo_mem; }
    //PRINT_MSG("file: %s , func :%s , line:%d  hop_size :%d\n",__FILE__ , __func__ , __LINE__ ,hop_size);

    do {
      // put some fresh data in input vector
      aubio_source_do_mem(source_mem, in_mem, &read); //read  = hop_size
      // execute tempo
      //PRINT_MSG("file: %s , func :%s , line:%d  hop_size :%d\n",__FILE__ , __func__ , __LINE__ ,hop_size);
      aubio_tempo_do(o,in_mem,out_mem);
      
      //PRINT_MSG("file: %s , func :%s , line:%d ,%d , %x\n",__FILE__ , __func__ , __LINE__,in_mem->length , *(in_mem->data));
      // do something with the beats
      if (out_mem->data[0] != 0) {//自己造的数据，是不会有节拍的
        PRINT_MSG("beat at %.3fms, %.3fs, frame %d, %.2f bpm "
            "with confidence %.2f\n",
            aubio_tempo_get_last_ms(o), aubio_tempo_get_last_s(o),
            aubio_tempo_get_last(o), aubio_tempo_get_bpm(o),
            aubio_tempo_get_confidence(o));
      }
      n_frames += read;
      //    PRINT_MSG("n_frames %d\n", n_frames);
    } while ( read == hop_size );

    PRINT_MSG("read %.2fs, %d frames at %dHz (%d blocks) from  P\n",
        n_frames * 1. / samplerate,
        n_frames, samplerate,
        n_frames / hop_size);

#if ZIJIZAO_MEM
free(p);

#else
free(p->data_addr);

#endif
   

    beach_tempo_mem:
    PRINT_ERR("read !!!!!!!!!!!!!!!\n");
      del_fvec(in_mem);
      del_fvec(out_mem);
      del_aubio_source(source_mem);
#ifdef ZIJIZAO_MEM
      free(p);
      
#else
      free(p->data_addr);
      
#endif


    // clean up memory
    del_aubio_tempo(o);
    beach:
      aubio_cleanup();

}
#endif 

#if FILE_INTERFACE
beach_tempo:
  del_fvec(in);
  del_fvec(out);
  del_aubio_source(source);
beach:
  aubio_cleanup();
#endif
  return err;
}
#pragma GCC pop_options //防止GDB时，被优化

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
