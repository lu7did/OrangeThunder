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



//*---------------------------------------------------------------------------------------------------
//* VFOSystem CLASS
//*---------------------------------------------------------------------------------------------------
class genVFO
{
  public: 
  
      genVFO(CALLBACK c);

      void set(byte v,float freq);
      void set(float freq);

      float get(byte v);
      float get();

      byte  vfo=VFOA;      
      byte  FT817=0x00;

      void setSplit(bool b);
      void setRIT(bool b);
      void setPTT(bool b);

      void setStep(byte stepIndex);

      void  getStr(byte bVFO);
      float update(int dir);
      float update(byte v,int dir);

      long int code2step(byte b);
      byte step2code(long int s);

      FSTR     vfostr[VFOMAX];
      CALLBACK changeVFO=NULL;

  
  private:


      void f2str(float f, FSTR* v);


      float loFreq[BANDMAX+1]={472,500,1800,3500,7000,10100,14000,18068,21000,24890,28000,50000,87000,108000,144000,420000,1240000};
      float hiFreq[BANDMAX+1]={479,1650,2000,3800,7300,10150,14350,18168,21450,24990,29700,54000,107000,137000,148000,450000,1300000};
      byte  isTxOk[BANDMAX+1]={  1,   0,   1,   1,   1,    1,    1,    1,    1,    1,    1,    1,     0,     0,     1,     1,      0};


      float stepList[12]={VFO_STEP_1Hz,VFO_STEP_10Hz,VFO_STEP_100Hz,VFO_STEP_500Hz,VFO_STEP_1KHz,VFO_STEP_5KHz,VFO_STEP_10KHz,VFO_STEP_50KHz,VFO_STEP_100KHz,VFO_STEP_500KHz,VFO_STEP_1MHz};

      float f[VFOMAX];
      float step[VFOMAX];
      float rit[VFOMAX];
      float shift[VFOMAX];

      byte  band=B40M;

};

#endif
//*---------------------------------------------------------------------------------------------------
//* VFO CLASS Implementation
//*---------------------------------------------------------------------------------------------------
genVFO::genVFO(CALLBACK c)
{
  
  if (c!=NULL) {this->changeVFO=c;}   //* Callback of change VFO frequency

  setWord(&FT817,VFO,VFOA);
  setWord(&FT817,PTT,0);
  setWord(&FT817,RIT,false);
  setWord(&FT817,SPLIT,false);
  setWord(&FT817,SHIFT,false);

  this->band=B20M;
  this->f[VFOA]=14074000;  
  this->f[VFOB]=14074000;  
  this->vfo=VFOA;
   

}
//*---------------------------------------------------------------------------------------------------
//* CLASS Implementation
//*---------------------------------------------------------------------------------------------------

void genVFO::set(byte v,float freq) {

   if (v!=VFOA && v!=VFOB) {return;}

   f[v]=freq;  
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

   return this->get(vfo);
}
//*---------------------------------------------------------------------------------------------------
//* CLASS Implementation
//*---------------------------------------------------------------------------------------------------

void genVFO::setSplit(bool b) {

   setWord(&FT817,SPLIT,b);
   return;
}
//*---------------------------------------------------------------------------------------------------
//* CLASS Implementation
//*---------------------------------------------------------------------------------------------------

void genVFO::setRIT(bool b) {
   setWord(&FT817,RIT,b);
   return;
}
//*---------------------------------------------------------------------------------------------------
//* CLASS Implementation
//*---------------------------------------------------------------------------------------------------

void genVFO::setPTT(bool b) {

   setWord(&FT817,PTT,b);
   return;
}
//*---------------------------------------------------------------------------------------------------
//* CLASS Implementation
//*---------------------------------------------------------------------------------------------------
void setStep(byte v, byte stepIndex) {

   if (v!=VFOA && v!=VFOB) {
      return;
   }

   return;
}
//*---------------------------------------------------------------------------------------------------
//* CLASS Implementation
//*---------------------------------------------------------------------------------------------------
void setStep(byte stepIndex) {


   return;
}

//*---------------------------------------------------------------------------------------------------
//* CLASS Implementation
//*---------------------------------------------------------------------------------------------------

void genVFO::getStr(byte bVFO) {
   return;
}
//*---------------------------------------------------------------------------------------------------
//* CLASS Implementation
//*---------------------------------------------------------------------------------------------------

float genVFO::update(byte b,int dir) {
   return 0.0;
}
//*---------------------------------------------------------------------------------------------------
//* CLASS Implementation
//*---------------------------------------------------------------------------------------------------

float genVFO::update(int dir) {
   return this->update(this->vfo,dir);
}

//*---------------------------------------------------------------------------------------------------
//* Create change frequency callback
//*---------------------------------------------------------------------------------------------------


