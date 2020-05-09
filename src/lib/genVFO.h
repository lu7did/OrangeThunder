//*--------------------------------------------------------------------------------------------------
//* VFO Management Class   (HEADER CLASS)
//*--------------------------------------------------------------------------------------------------
//* Este es el firmware del diseÃ±o de VFO  para la plataforma OrangeThunder
//* Solo para uso de radioaficionados, prohibido su utilizacion comercial
//* Copyright 2018,2020 Dr. Pedro E. Colla (LU7DID)
//*--------------------------------------------------------------------------------------------------
#ifndef genVFO_h
#define genVFO_h

//*--- Definition for VFO parameters and limits

#define VFOA    0
#define VFOB    1
#define VFOMAX  2

#define  VLF    0
#define  LF     1
#define  B160M  2
#define  B80M   3
#define  B40M   4
#define  B30M   5
#define  B20M   6
#define  B17M   7
#define  B15M   8
#define  B12M   9
#define  B10M  10
#define  B6M   11
#define  BFM   12
#define  BAR   13
#define  B2M   14
#define  B70CM 15
#define  B23CM 16


#define VFO_STEP_1Hz            1
#define VFO_STEP_10Hz          10
#define VFO_STEP_100Hz        100
#define VFO_STEP_500Hz        500
#define VFO_STEP_1KHz        1000
#define VFO_STEP_5KHz        5000
#define VFO_STEP_10KHz      10000
#define VFO_STEP_50KHz      50000
#define VFO_STEP_100KHz    100000
#define VFO_STEP_500KHz    500000
#define VFO_STEP_1MHz     1000000

typedef unsigned char byte;
typedef bool boolean;
typedef void (*CALLBACK)();

bool getWord (unsigned char SysWord, unsigned char v);
void setWord(unsigned char* SysWord,unsigned char v, bool val);

typedef struct {
  byte ones;
  byte tens;
  byte hundreds;
  byte thousands;
  byte tenthousands;
  byte hundredthousands;
  byte millions;
 
} FSTR;

//*============================================================================================
//* Define band limits
//*============================================================================================
//*---- HF band definition

#define BANDMAX 20
#define STEPMAX 10


//*---------------------------------------------------------------------------------------------------
//* VFOSystem CLASS
//*---------------------------------------------------------------------------------------------------
class genVFO
{
  public: 
  
      genVFO(CALLBACK df,CALLBACK dm,CALLBACK ds);

      void set(byte v,float freq);
      void set(float freq);

      float get(byte v);
      float get();

      byte  vfo=VFOA;      

      byte  FT817=0x00;
      byte  TRACE=0x00;
      byte  MODE=MUSB;

      void  setSplit(bool b);
      float setPTT(bool b);

      void setRIT(byte v,bool b);
      void setRIT(bool b);

      void swapVFO();

      int  getBand(float f);
      void setBand(byte band);
      void setBand(byte v,byte band);

      void setStep(byte v,byte s);
      void setStep(byte s);

      char* vfo2str(byte v);

      float up();
      float down();
      float update(int dir);
      float update(byte v,int dir);

      float updateRIT(byte v,int dir);
      float updateRIT(int dir);

      float valueRIT(byte v);
      float valueRIT();

      bool  getSplit();
      bool  getRIT(byte v);
      bool  getRIT();

      void  setMode(byte v, byte m);
      void  setMode(byte m);
      byte  getMode();
      byte  getMode(byte m);
      long int code2step(byte b);
      byte step2code(long int s);

      FSTR vfostr[VFOMAX];
      CALLBACK changeVFO=NULL;
      CALLBACK changeMode=NULL;
      CALLBACK changeStatus=NULL;

  
  private:

const char   *PROGRAMID="genVFO";
const char   *PROG_VERSION="1.0";
const char   *PROG_BUILD="00";
const char   *COPYRIGHT="(c) LU7DID 2019,2020";



      void f2str(float f, FSTR* v);

      float loFreq[BANDMAX+1]={472,500,1800,3500,7000,10100,14000,18068,21000,24890,28000,50000,87000,108000,144000,420000,1240000};
      float hiFreq[BANDMAX+1]={479,1650,2000,3800,7300,10150,14350,18168,21450,24990,29700,54000,107000,137000,148000,450000,1300000};
      byte  isTxOk[BANDMAX+1]={  1,   0,   1,   1,   1,    1,    1,    1,    1,    1,    1,    1,     0,     0,     1,     1,      0};


      float stepList[12]={VFO_STEP_1Hz,VFO_STEP_10Hz,VFO_STEP_100Hz,VFO_STEP_500Hz,VFO_STEP_1KHz,VFO_STEP_5KHz,VFO_STEP_10KHz,VFO_STEP_50KHz,VFO_STEP_100KHz,VFO_STEP_500KHz,VFO_STEP_1MHz};

      float f[VFOMAX];
      float step[VFOMAX];
      float rit[VFOMAX];
      float shift[VFOMAX];
      float fmin[VFOMAX];
      float fmax[VFOMAX];
      byte  band[VFOMAX];
      byte  mode[VFOMAX];

};

#endif
//*---------------------------------------------------------------------------------------------------
//* VFO CLASS Implementation
//*---------------------------------------------------------------------------------------------------
genVFO::genVFO(CALLBACK df,CALLBACK dm,CALLBACK ds)
{
  
  if (df!=NULL) {this->changeVFO=df;}   //* Callback of change VFO frequency
  if (dm!=NULL) {this->changeMode=dm;}
  if (ds!=NULL) {this->changeStatus=ds;}

  setWord(&FT817,VFO,VFOA);
  setWord(&FT817,PTT,0);
  setWord(&FT817,RIT,false);
  setWord(&FT817,SPLIT,false);
  //setWord(&FT817,SHIFT,false);

  for (int i=0;i<VFOMAX;i++) {

     step[i]=VFO_STEP_100Hz;
     rit[i]=0.0;
     shift[i]=600.0;
     mode[i]=MCW;

  }

  this->setSplit(getWord(FT817,SPLIT));

  setBand(VFOA,getBand(14000000));
  setBand(VFOB,getBand(14000000));

  this->setRIT(VFOA,getWord(FT817,RIT));
  this->setRIT(VFOB,getWord(FT817,RIT));

  this->MODE=MUSB;


  this->set(VFOA,14074000);
  this->set(VFOB,14074000);
  this->vfo=VFOA;
  this->setPTT(false);
  (this->TRACE>=0x02 ? fprintf(stderr,"%s::genVFO() Initialization completed\n",PROGRAMID) : _NOP);   

}
//*---------------------------------------------------------------------------------------------------
//* CLASS Implementation (mode support)
//*---------------------------------------------------------------------------------------------------
byte genVFO::getMode(byte v){
   if (v!=VFOA && v!=VFOB) {return VFOA;}
   return this->mode[v];
}
byte genVFO::getMode() {
     return getMode(this->vfo);
}
void genVFO::setMode(byte v,byte m) {
   if (v!=VFOA && v!=VFOB) {return;}
   if (m!=MCW && m!=MCWR && m!=MDIG) {
      return;
   }
   mode[v]=m;
   if (changeMode!=NULL) {changeMode();}
}
void genVFO::setMode(byte m) {
   return this->setMode(this->vfo,m);
}
//*---------------------------------------------------------------------------------------------------
//* CLASS Implementation
//*---------------------------------------------------------------------------------------------------
void genVFO::set(byte v,float freq) {

   if (v!=VFOA && v!=VFOB) {return;}
   if (freq<loFreq[band[v]]*1000 || freq>hiFreq[band[v]]*1000) {
      (this->TRACE>=0x02 ? fprintf(stderr,"%s::set() VFO[%s] **ERROR** freq(%f) fmin(%f) fmax(%f) band(%d)\n",PROGRAMID,vfo2str(v),freq,fmin[v],fmax[v],band[v]) : _NOP);   
      return;
   }
   f[v]=freq;  
  (this->TRACE>=0x02 ? fprintf(stderr,"%s::set() VFO[%s] freq(%f) fmin(%f) fmax(%f) band(%d)\n",PROGRAMID,vfo2str(v),f[v],fmin[v],fmax[v],band[v]) : _NOP);   
   if (changeVFO!=NULL) {changeVFO();}
   return ;
}
//*---------------------------------------------------------------------------------------------------
//* CLASS Implementation
//*---------------------------------------------------------------------------------------------------
void genVFO::set(float freq) {

   this->set(vfo,freq);
   return;
}
//*---------------------------------------------------------------------------------------------------
//* CLASS Implementation
//*---------------------------------------------------------------------------------------------------
float genVFO::get(byte v) {

   if (v!=VFOA && v!=VFOB) {
      return f[VFOA];
   }
   return f[v];
}
//*---------------------------------------------------------------------------------------------------
//* CLASS Implementation
//*---------------------------------------------------------------------------------------------------
float genVFO::get() {

   return this->get(this->vfo);
}
//*---------------------------------------------------------------------------------------------------
//* CLASS Implementation
//*---------------------------------------------------------------------------------------------------

void genVFO::setSplit(bool b) {
   setWord(&FT817,SPLIT,b);
  (this->TRACE>=0x02 ? fprintf(stderr,"%s::setSplit() VFO[%s] Split[%s]\n",PROGRAMID,vfo2str(this->vfo),BOOL2CHAR(getWord(FT817,SPLIT))) : _NOP);   
   if (changeStatus!=NULL) {changeStatus();}
   return;
}
//*---------------------------------------------------------------------------------------------------
//* CLASS Implementation
//*---------------------------------------------------------------------------------------------------

void genVFO::setRIT(byte v,bool b) {
   if(v !=VFOA && v != VFOB) {
     return;
   }

   setWord(&FT817,RIT,b);
  (this->TRACE>=0x02 ? fprintf(stderr,"%s::setRIT() VFO[%s] RIT[%s] RitOffset[%f]\n",PROGRAMID,vfo2str(v),BOOL2CHAR(getWord(FT817,RIT)),rit[v]) : _NOP);   
   if (changeStatus!=NULL) {changeStatus();}
   return;
}
void genVFO::setRIT(bool b) {
   this->setRIT(vfo,b);
   return;
}
//*---------------------------------------------------------------------------------------------------
//* CLASS Implementation
//*---------------------------------------------------------------------------------------------------
float genVFO::setPTT(bool b) {

byte  v;
float o;


   if (getWord(FT817,SPLIT)==true) {
      (vfo==VFOA ? v=VFOB : v=VFOA);
    } else {
       v=vfo;
    }


   if (b==false) {   // compute reception frequency
      vfo=v;
      (getWord(FT817,RIT)==true ? o=rit[vfo] : o=0.0);
      setWord(&FT817,PTT,false);
      o=o+f[vfo];
      (this->TRACE>=0x02 ? fprintf(stderr,"%s::setPTT()  PTT[%s] frequency(%f)\n",PROGRAMID,BOOL2CHAR(getWord(FT817,PTT)),o) : _NOP);   
      if (changeStatus!=NULL) {changeStatus();}
      return o;
   }

// from now on is transmittion frequency

   vfo=v;
   float s=0.0;
   if (MODE==MCW) {
      s=shift[vfo];
   } else {
     if (MODE==MCWR) {
        s=-1*shift[vfo];
     }
   }
   o=f[vfo]+s;
   setWord(&FT817,PTT,b);
  (this->TRACE>=0x02 ? fprintf(stderr,"%s::setPTT()  PTT[%s] frequency(%f)\n",PROGRAMID,BOOL2CHAR(getWord(FT817,PTT)),o) : _NOP);   
  if (changeStatus!=NULL) {changeStatus();}
  return o;
}
//*---------------------------------------------------------------------------------------------------
//* CLASS Implementation
//*---------------------------------------------------------------------------------------------------
void genVFO::setStep(byte v, byte s) {

   if (v!=VFOA && v!=VFOB) {
      return;
   }
   if (s<0 || s>=STEPMAX) {
      return;
   }
   step[v]=stepList[s];
  (this->TRACE>=0x02 ? fprintf(stderr,"%s::setStep() VFO[%s] Code[%d] Step[%f]\n",PROGRAMID,vfo2str(v),s,step[v]) : _NOP);   
  if (changeStatus!=NULL) {changeStatus();}
  return;
}
//*------------------------------------------------------------------------------------------------------
//* Flip VFOA/VFOB
//*------------------------------------------------------------------------------------------------------
void genVFO::swapVFO() {

   if (this->vfo==VFOA) {
      this->vfo = VFOB;
      if (changeStatus!=NULL) {changeStatus();}
      return;
   }
   this->vfo=VFOA;
   if (changeStatus!=NULL) {changeStatus();}

   return;
}
//*---------------------------------------------------------------------------------------------------
//* CLASS Implementation
//*---------------------------------------------------------------------------------------------------
void genVFO::setStep(byte s) {

   return setStep(vfo,s);
}

//*---------------------------------------------------------------------------------------------------
//* CLASS Implementation
//*---------------------------------------------------------------------------------------------------

char* genVFO::vfo2str(byte v) {
   return (v==VFOA ? (char*)"VfoA" : (char*)"VfoB");
}
//*---------------------------------------------------------------------------------------------------
//* CLASS Implementation
//*---------------------------------------------------------------------------------------------------

void genVFO::setBand(byte v,byte band) {

  //this->set(v,loFreq[band]*1000);
  this->band[v]=band;
  this->fmin[v]=loFreq[band]*1000;
  this->fmax[v]=hiFreq[band]*1000;
  this->set(v,fmin[v]);
  (this->TRACE>=0x02 ? fprintf(stderr,"%s::setBand() VFO[%s] Code[%d] fmin[%f fmax[%f] f[%f]\n",PROGRAMID,vfo2str(v),band,loFreq[band]*1000,hiFreq[band]*1000,f[v]) : _NOP);   
   return;
}

//*---------------------------------------------------------------------------------------------------
//* CLASS Implementation
//*---------------------------------------------------------------------------------------------------

void genVFO::setBand(byte band) {

   this->setBand(vfo,band);
   return;
}
//*---------------------------------------------------------------------------------------------------
//* getBand
//* given the frequency obtain the band (-1 if invalid)
//*---------------------------------------------------------------------------------------------------
int genVFO::getBand(float freq) {

   for(int i=0;i<BANDMAX;i++) {
      if (freq>=loFreq[i]*1000 && freq<=hiFreq[i]*1000) {
         (this->TRACE>=0x02 ? fprintf(stderr,"%s::getBand()  freq[%f] band[%d]\n",PROGRAMID,freq,i) : _NOP);   
         return i;
      }
   }
   (this->TRACE>=0x02 ? fprintf(stderr,"%s::getBand()  ***ERROR*** freq[%f] has no authorized band\n",PROGRAMID,freq) : _NOP);   
   return -1;     

}

//*---------------------------------------------------------------------------------------------------
//* CLASS Implementation
//*---------------------------------------------------------------------------------------------------

float genVFO::update(byte v,int dir) {

   float freq=f[v];
   f[v]=f[v]+dir*step[v];

   if (f[v] > fmax[v]) {
      f[v]=fmax[v];
   }

   if (f[v] < fmin[v]) {
      f[v]=fmin[v];
   }

   (this->TRACE>=0x02 ? fprintf(stderr,"%s::update() VFO[%s] dir[%d] f[%f]->[%f]\n",PROGRAMID,vfo2str(v),dir,freq,f[v]) : _NOP);     
   if (changeVFO!=NULL) {changeVFO();}
   return f[v];
}
//*---------------------------------------------------------------------------------------------------
//* CLASS Implementation
//*---------------------------------------------------------------------------------------------------
float genVFO::update(int dir) {

   return this->update(this->vfo,dir);
}
//*---------------------------------------------------------------------------------------------------
//* CLASS Implementation
//*---------------------------------------------------------------------------------------------------

float genVFO::up() {
   return this->update(+1);
}
//*---------------------------------------------------------------------------------------------------
//* CLASS Implementation
//*---------------------------------------------------------------------------------------------------
float genVFO::down() {
   return this->update(-1);
}
//*---------------------------------------------------------------------------------------------------
//* CLASS Implementation
//*---------------------------------------------------------------------------------------------------
float genVFO::valueRIT(byte v) {

   if (v<0 || v>VFOMAX) {
      return 0.0;
   }

   if (this->getRIT(v)==false) {
      return 0.0;
   }

   return rit[v];

}
//*---------------------------------------------------------------------------------------------------
//* CLASS Implementation
//*---------------------------------------------------------------------------------------------------
float genVFO::valueRIT() {
   return valueRIT(this->vfo);
}
//*---------------------------------------------------------------------------------------------------
//* CLASS Implementation
//*---------------------------------------------------------------------------------------------------
float genVFO::updateRIT(byte v,int dir) {

    if (v<0 || v>VFOMAX) {
       return 0.0;
    }
float d=dir*VFO_STEP_100Hz;

    if ( abs(rit[v]+d)>999.0 ){
      
    } else {
      rit[v]=rit[v]+d;
    }
    if (changeVFO!=NULL) {changeVFO();}
    return rit[v];
}
//*---------------------------------------------------------------------------------------------------
//* CLASS Implementation
//*---------------------------------------------------------------------------------------------------
float genVFO::updateRIT(int dir) {

   return updateRIT(vfo,dir);
}
//*---------------------------------------------------------------------------------------------------
//* CLASS Implementation
//*---------------------------------------------------------------------------------------------------
bool  genVFO::getSplit() {

    return getWord(FT817,SPLIT);
}
//*---------------------------------------------------------------------------------------------------
//* CLASS Implementation
//*---------------------------------------------------------------------------------------------------
bool  genVFO::getRIT(byte v) {

    return getWord(FT817,RIT);
}
//*---------------------------------------------------------------------------------------------------
//* CLASS Implementation
//*---------------------------------------------------------------------------------------------------
bool  genVFO::getRIT() {

    return getRIT(vfo);
}
//*---------------------------------------------------------------------------------------------------
//* Create change frequency callback
//*---------------------------------------------------------------------------------------------------


