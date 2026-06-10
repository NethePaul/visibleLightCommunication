#include <bitset>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include<iostream>
#include <vector>
#include"opencv2/core.hpp"
#include "opencv2/core/base.hpp"
#include "opencv2/core/hal/interface.h"
#include "opencv2/core/mat.hpp"
#include "opencv2/core/types.hpp"
#include "opencv2/imgproc.hpp"
#include"opencv2/videoio.hpp"
#include"opencv2/highgui.hpp"
#include"opencv2/core/ocl.hpp"

static int min_thresh=130;
static int max_thresh=255;
static int neighbourhood=4;

struct BitStream{
  cv::Point loc;
  std::string str="";
  std::string bits="";
  size_t last_frame_id=0;
  size_t last_last_frame_id=0;
  uint64_t last_bits=0;
  double last_v=0;
  char zero=0;
  char data=0;
  bool has_data=0;
  bool receiving_data=0;
  bool start=0;
  bool end=0;
  bool has_unprocessed_bit=0;
  uint8_t last_delta=0;
  double last_sub_delta=0;
  double last_last_sub_delta=0;
  int8_t c=0;
  enum{RISING, FALLING, NOTHING, UNINITIALIZED} rising=UNINITIALIZED;
};

int decodeDelta(double delta){
  int r=-1;
  if(delta>0 && delta<=90){
    r=0;
  }
  else if(delta>90 && delta<=200){
    r=1;
  }
  else {
    r=-1;
  }
  return r;
}

void decodeBit(BitStream&stream, double sub_delta, int delta, size_t frame_id, cv::Point loc, double v, bool rising){
  int b=decodeDelta(delta + sub_delta - stream.last_sub_delta);
  std::cout<<"exact delta: "<<(delta + sub_delta - stream.last_sub_delta)<<"  at: "<<frame_id<<" with: "<<v<<std::endl;
  std::cout<<"drift: "<<(sub_delta - stream.last_sub_delta)<<std::endl;
  std::cout<<"subdelta: "<<(sub_delta)<<std::endl;
  if(b>=0){
    stream.last_bits<<=1;
    stream.last_bits|=b;
    stream.bits+=std::to_string(b);
    stream.has_unprocessed_bit=1;
    stream.rising=rising?stream.RISING:stream.FALLING;
  }else{
    stream.rising=stream.NOTHING;
    stream.receiving_data=0;
  }
  stream.last_delta=delta;
  stream.last_last_sub_delta=stream.last_sub_delta;
  stream.last_sub_delta=sub_delta;
  stream.last_v=v;
  stream.loc=loc;
  stream.last_last_frame_id=stream.last_frame_id;
  stream.last_frame_id=frame_id;
}

void reviseBit(BitStream&stream, int&delta, size_t frame_id, bool rising, double v){
  stream.last_frame_id=stream.last_last_frame_id;
  stream.last_bits>>=1;
  stream.bits=stream.bits.substr(0, stream.bits.length()-1);
  stream.has_unprocessed_bit=0;
  stream.last_sub_delta=stream.last_last_sub_delta;

  stream.last_v=v;
  delta=frame_id-stream.last_frame_id;
}

int hamming_distance(int a, int b){
  int d=0;
  for(size_t i=0; i<sizeof(int)*8; i++){
    if(((a>>i)&1) != ((b>>i)&1))
      d++;
  }
  return d;
}


void decodeData(BitStream&stream){
  if(!stream.has_unprocessed_bit)return;
  stream.has_unprocessed_bit=0;
  if(!stream.receiving_data && (stream.last_bits&0xFFF) == 0xFFF){//start
    stream.start=1;
    stream.end=0;
    stream.c=0;
    stream.str="";
    stream.bits="";
    stream.receiving_data=1;
    std::cout<<"started receiving"<<std::endl;
  }
  else if(stream.receiving_data){
    stream.c++;
    if(stream.c>=8){
      stream.c=0;
      stream.data=(stream.last_bits&0xFF);
      stream.has_data=1;
      if(stream.data==0){//end
        stream.end=1;
        stream.receiving_data=0;
      }
      else{
        stream.str+=stream.data;
        std::cout<<" received bits: "+stream.bits<<std::endl;
        std::cout<<" received data: "<<stream.data<<std::endl;
        stream.bits="";
      }
    }
  }
}

bool decodeFrame(BitStream&stream, double sub_delta, size_t frame_id, bool rising, cv::Point loc, double v){
  stream.start=0;
  stream.end=0;
  stream.has_data=0;
  if(!(loc-stream.loc).inside(cv::Rect(-neighbourhood/2-1,-neighbourhood/2-1,neighbourhood+2,neighbourhood+2)) && stream.rising!=BitStream::UNINITIALIZED){
    return false;
  }
  int delta=frame_id-stream.last_frame_id;
  if(delta==0) return false;
  

  if( v>stream.last_v && stream.has_unprocessed_bit && (
    ( rising && stream.rising==stream.RISING ) ||
    (!rising && stream.rising==stream.FALLING)
  )){
    reviseBit(stream, delta, frame_id, rising, v);
    decodeBit(stream, sub_delta, delta, frame_id, loc, v, rising);
    return true;
  }

  decodeData(stream);

  if(
    ( rising && stream.rising!=stream.RISING ) ||
    (!rising && stream.rising!=stream.FALLING)
  ){
    decodeBit(stream, sub_delta, delta, frame_id, loc, v, rising);
  }
  else if(frame_id-stream.last_frame_id>11){
    stream.rising=stream.NOTHING;
  }
  return true;
}

struct Point{cv::Point p; bool rising; double v; double sub_delta;};
void find_points(std::vector<Point>&points, bool rising, cv::Mat&frame, cv::Mat&frame_previous, cv::Mat&frame_sum, size_t delta){
  cv::Point p;
  double v=0;
  for(int i=0;i<100;i++){
    cv::minMaxLoc(frame_sum,0,&v,0,&p);
    cv::rectangle(frame_sum, p-cv::Point{neighbourhood/2+1,neighbourhood/2+1}, p+cv::Point{neighbourhood/2+1,neighbourhood/2+1}, 0, cv::FILLED);
    if(v>=min_thresh && v<=max_thresh){
      double v1 = frame_previous.at<uint8_t>(p);
      double v2 = frame.at<uint8_t>(p);
      double sub_delta = -v1/(v1+v2)*delta;
      points.push_back({p,rising,v,sub_delta});
    }
    else break;
  }
}

int main(int argc,const char**argv,const char**env){
  if(argc==2&&std::string(argv[1]).find("-h")<2){
    std::cout<<"Usage: decoder [filename [min_threshold [max_threshold ]]]"<<std::endl;
    std::cout<<"filename     : str, (default: video.mp4), (eg: /dev/video0)"<<std::endl;
    std::cout<<"min_threshold: int 0<=x<=255, (default: 130)"<<std::endl;
    std::cout<<"max_threshold: int 0<=x<=255, (default: 255)"<<std::endl;
    return 0;
  }
  std::string filename="video.mp4";
  if(argc>=2){filename=argv[1];}
  if(argc>=3){min_thresh=std::atoi(argv[2]);}
  if(argc>=4){max_thresh=std::atoi(argv[3]);}
  if(argc>=5){neighbourhood=std::atoi(argv[4]);}
  
  cv::VideoCapture cap(filename);
  bool first_iteration=true;
  cv::Mat //change to UMAT when on iGPU, UMAT does not work on dGPU, due to high latency
    frame_previous,
    frame_current,
    frame_rising,
    frame_falling,
    frame_rising_thresh,
    frame_falling_thresh,
    comparison;
  cv::Mat
    cpu_frame_rising,
    cpu_frame_falling,
    cpu_frame_sum_rising,
    cpu_frame_sum_falling,
    cpu_frame_previous_rising,
    cpu_frame_previous_falling;
  
  cv::Mat dilate_kernel(cv::Size(neighbourhood, neighbourhood), CV_8U);
 
  if(cv::ocl::haveOpenCL())
    cv::ocl::setUseOpenCL(true);
 
  if(!cap.read(frame_previous))return 1;
  cv::cvtColor(frame_previous, frame_previous, cv::COLOR_RGB2GRAY);

  std::vector<BitStream>streams;

  size_t last_pos=0;
  while(true){
    size_t pos=cap.get(cv::CAP_PROP_POS_MSEC);
    std::cout<<"prop ms: "<<pos-last_pos<<std::endl;

    frame_current.release();
    cpu_frame_falling.release();
    cpu_frame_rising.release();
    frame_falling.release();
    frame_rising.release();
    
    if(!cap.read(frame_current))return 1;

    cv::imshow("Orig",frame_current);

    cv::cvtColor(frame_current, frame_current, cv::COLOR_RGB2GRAY);
    
    cv::subtract(frame_current , frame_previous, frame_rising );//diff(f_n, f_{n-1})
    cv::subtract(frame_previous, frame_current , frame_falling);

    cv::dilate(frame_rising , frame_rising , dilate_kernel);//prepare for downscale, without loosing largest signal
    cv::dilate(frame_falling, frame_falling, dilate_kernel);

    double downscale=neighbourhood;
    cv::resize(frame_falling, cpu_frame_falling, cv::Size(), 1.f/downscale, 1.f/downscale, cv::INTER_NEAREST);//downscale
    cv::resize(frame_rising , cpu_frame_rising , cv::Size(), 1.f/downscale, 1.f/downscale, cv::INTER_NEAREST);

    if(!first_iteration){
      cv::add(cpu_frame_rising , cpu_frame_previous_rising , cpu_frame_sum_rising );//diff(f_n, f_{n-2})
      cv::add(cpu_frame_falling, cpu_frame_previous_falling, cpu_frame_sum_falling);

      std::vector<Point>points;
      find_points(points, true, cpu_frame_rising  , cpu_frame_previous_rising , cpu_frame_sum_rising , pos-last_pos);
      find_points(points, false, cpu_frame_falling, cpu_frame_previous_falling, cpu_frame_sum_falling, pos-last_pos);
    
      bool any=false;
      for(auto&point:points){
        if(point.rising){
          cpu_frame_sum_rising.at<uint8_t>(point.p)=255;
        }else{
          cpu_frame_sum_rising.at<uint8_t>(point.p)=150;
        }
        int i=0;
        for(auto&stream:streams){i++;
          if(decodeFrame(stream, point.sub_delta, pos, point.rising, point.p, point.v)){
            std::cout<<"\n\nstream: "<<i<<" at: "<<point.p.x<<" bits: "<<std::bitset<64>(stream.last_bits)<<std::endl;
            if((stream.end)){
              std::cout<<"\n\nstream: "<<i<<" at: "<<point.p.x<<" received string: "+stream.str<<std::endl;
            }
            any=true;
            break;
          }
        }
        if(!any){
          streams.emplace_back();
          decodeFrame(streams.back(), point.sub_delta, pos, point.rising, point.p, point.v);
        }
      }
      cv::imshow("Live",cpu_frame_sum_rising);
      if(cv::pollKey()=='q')return 0;
    }
    
    frame_previous=frame_current;
    cpu_frame_previous_falling=cpu_frame_falling;
    cpu_frame_previous_rising=cpu_frame_rising;
    first_iteration=false;
    last_pos=pos;
  }
}
