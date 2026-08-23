#include "Wire.h"
#include "I2Cdev.h"
#include "MPU6050_6Axis_MotionApps20.h"
#include "SD.h"
#include <SoftwareSerial.h>
#include <SFE_BMP180.h>
#include <avr/wdt.h>
// #include <stdio.h>
// #include <stdlib.h>
#include <math.h>


SFE_BMP180 pressure;



#define gpsSerial  SerialGPS
#define ALTITUDE 193.0 //请输入发射地的海拔
#define Vertical_acceleration_judgment_value0 21 //设置加速度为多少时判断火箭为已发射的值，本人经验值为15.（snowflake001的经验值为20）       
#define Launched 1                              //用来描述火箭状态为已发射.
#define Not_launched 0                          //用来描述火箭状态为尚未发射.
#define Parachute_ignition_switch 6             //用来定义火箭开伞继电器IN1口接在NANO的D3口上
#define Connect LOW                            //开伞点火器接通值.
#define Disconnect HIGH                          //开伞点器断开值.
#define Open_the_parachute_inclination 120    //设置箭体与地平线的倾角达到多少度时开伞，可以根据你想要的开伞角度来调整该数值，-10度时火箭刚刚过顶点开始下降约五、六米左右
#define The_correct_flight_angle_limit_of_rocket 30 //设置火箭正常发射的角度极值
SoftwareSerial SerialGPS(3, 7); // 定义GPS的端口
SoftwareSerial mySerial(10, 9);//定义LORA的端口

unsigned long time;
int16_t ax, ay, az;//三轴加速度
const int chipSelect = 4;  //设定SD_CS接口
bool DiditLaunch = false;//该布尔值代表火箭是否发射（Snow）

int OPEN;
int rocket_status;                              //该变量代表火箭当前的状态，分别为未发射或已发射。
float inclination[3];                           //箭体三轴倾角,是一个数组,inclination[0]  inclination[1]  inclination[2] 分别代表三个轴的倾角
int zz,yy,xx,Current_rocket_inclination,aaa,Aint;                 //该变量代表火箭与地平线的倾角.zz和yy是后加上去了，三个轴都采集了，可以再现火箭整个飞行过程中的箭体三维姿态。Current_rocket_inclination
int16_t Vertical_acceleration;                  //火箭垂直加速度,用来判断火箭是否已发射.
int last_rocket_inclination=0;                  //上一次通过传感器获得的火箭倾角.
//下面为计算三轴角度的一些中间变量
MPU6050 mpu;        
uint8_t a1;
uint8_t DiditBomb;
uint16_t a2;
uint8_t currentAngle = 0;
uint16_t a3;
uint8_t a4[64];
Quaternion a5;
VectorFloat a6;
//上面为计算三轴角度的一些中间变量
double Hight = 150; //(输入发射地点的海拔,单位米)
// bool dmpReady = false;//4
// uint16_t packetSize;//4
// uint16_t fifoCount;//4
// uint8_t fifoBuffer[64];//4
// Quaternion q;//4
// VectorFloat gravity;//4
float w_float, x_float, y_float, z_float;//4
// int L = 13; //LED指示灯引脚GPS
// const unsigned int gpsRxBufferLength = 600;//GPS
// char gpsRxBuffer[gpsRxBufferLength];//GPS
// unsigned int ii = 0;//GPS

// struct
// {
// 	char GPS_Buffer[80];
// 	bool isGetData;		//是否获取到GPS数据
// 	bool isParseData;	//是否解析完成
// 	char UTCTime[11];		//UTC时间
// 	char latitude[11];		//纬度
// 	char N_S[2];		//N/S
// 	char longitude[12];		//经度
// 	char E_W[2];		//E/W
// 	bool isUsefull;		//定位信息是否有效
// } Save_Data;

void setup()
{  
    char status;
    double T,P,p0,Altitude;
    DiditBomb=0;
    OPEN=0;
    pinMode(Parachute_ignition_switch,OUTPUT);          //设置NANO上D3口作为开伞点火开关的输出口.
    digitalWrite(Parachute_ignition_switch,Disconnect); //将开伞点火开关初始化为断开.
    Wire.begin();

    Serial.begin(9600);
    mySerial.begin(9600);
   
    Init_MPU_6050();                                    //初始化MPU6050.
    rocket_status=Not_launched;                         //将火箭状态初始化为未发射.
    SD.begin(chipSelect);
    clearSDCard();
    pressure.begin();
    // Save_Data.isGetData = false;
	  // Save_Data.isParseData = false;
	  // Save_Data.isUsefull = false;
    uint8_t devStatus = mpu.dmpInitialize();

   
    // mpu.setXAccelOffset(24);
    // mpu.setYAccelOffset(26);
    // mpu.setZAccelOffset(1799); 
    // mpu.setXGyroOffset(-12);
    // mpu.setYGyroOffset(5);
    // mpu.setZGyroOffset(-17);
   
    // mpu.setXGyroOffset(199);
    // mpu.setYGyroOffset(95);
    // mpu.setZGyroOffset(-109);
    // mpu.setXAccelOffset(-224);
    // mpu.setYAccelOffset(-598);
    // // mpu.setXAccelOffset(-229);
    // // mpu.setYAccelOffset(-584);
    // mpu.setZAccelOffset(1799);
 
 
    // mpu.setXGyroOffset(199);
    // mpu.setYGyroOffset(95);
    // mpu.setZGyroOffset(-109);
    // mpu.setXAccelOffset(-212);
    // mpu.setYAccelOffset(-577);
    // mpu.setZAccelOffset(1799);
   
   

        if (devStatus == 0) {
        // 开启DMP
        mpu.setDMPEnabled(true);
    } else {
        Serial.println("DMP初始化失败");
        while (1);
    }
    wdt_enable(WDTO_4S); //看门狗  
    status = pressure.startTemperature();
    status = pressure.getTemperature(T);
    status = pressure.startPressure(3);
    status = pressure.getPressure(P,T);
    p0 = pressure.sealevel(P,ALTITUDE);
    Hight = pressure.altitude(P,p0);
}

Quaternion conjugate(const Quaternion& q) {
    return {q.w, -q.x, -q.y, -q.z};
}
 
// 四元数乘法
Quaternion multiply(const Quaternion& q1, const Quaternion& q2) {
    return {
        q1.w * q2.w - q1.x * q2.x - q1.y * q2.y - q1.z * q2.z,
        q1.w * q2.x + q1.x * q2.w + q1.y * q2.z - q1.z * q2.y,
        q1.w * q2.y - q1.x * q2.z + q1.y * q2.w + q1.z * q2.x,
        q1.w * q2.z + q1.x * q2.y - q1.y * q2.x + q1.z * q2.w
    };
}

// 使用四元数旋转向量
Quaternion rotateVector(const Quaternion& q, const Quaternion& v) {
    Quaternion q_conj = conjugate(q);
    Quaternion qv = multiply(multiply(q, v), q_conj);
    return qv;
}

// 计算向量的模
float magnitude(const Quaternion& v) {
    return sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
}

// 计算向量点积
float dotProduct(const Quaternion& v1, const Quaternion& v2) {
    return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;
}

// 计算夹角
float calculateAngle(const Quaternion& q) {
    Quaternion v = {0, 0, 0, 1}; // 飞机法线向量
    Quaternion v_rotated = rotateVector(q, v);
    Quaternion v_ground = {0, 0, 0, 1}; // 地面法线向量

    float dot = dotProduct(v_rotated, v_ground);
    float mag_v_rotated = magnitude(v_rotated);
    float mag_v_ground = magnitude(v_ground);

    float cos_theta = dot / (mag_v_rotated * mag_v_ground);
    float theta = acos(cos_theta);
    return theta * 180.0 / PI; // 将弧度转换为角度
}

bool dmpReady = false;
uint16_t packetSize;
uint16_t fifoCount;
uint8_t fifoBuffer[64];

Quaternion q;
VectorFloat gravity;

void loop(){
  // char status;
  
  float Vertical_accelerationY; // 声明Y轴加速度变量
  float Vertical_accelerationZ; // 声明Z轴加速度变量
  float Vertical_acceleration;  // 声明X轴加速度变量 
  ReadAndParseQuaternion();
  ReadHight();
  sendHight(); 
  SendDualQuaternion() ;
  Vertical_acceleration=mpu.getAccelerationX()/2048.0;//还是因为传感器安装位置的关系，火箭垂直轴为X轴。这里表示的是当前火箭的垂直加速度，程序里用作判断火箭是否已经发射。其实就是表示X轴的当前加速度/2048.0
  Vertical_accelerationY=mpu.getAccelerationY()/2048.0;
  Vertical_accelerationZ=mpu.getAccelerationZ()/2048.0;
  Serial.println(Vertical_accelerationZ);
  if (Vertical_accelerationZ>=Vertical_acceleration_judgment_value0) {
     DiditLaunch= true;

  }
  Serial.println("OK0");  
  if (DiditLaunch) {
    
    SDwrite(w_float, x_float, y_float, z_float, Vertical_acceleration, Vertical_accelerationY, Vertical_accelerationZ, Aint);//SD卡写入 
    Serial.println("OK1");
  
        if (currentAngle > Open_the_parachute_inclination) { // 判断箭体倾角是否达到程序开头所设置的开伞倾角,如果达到开伞倾角，则进入下面的倾角再判断。
        mpu.dmpGetQuaternion(&q, fifoBuffer);
        mpu.dmpGetGravity(&gravity, &q);

        w_float = q.w;
        x_float = q.x;
        y_float = q.y;
        z_float = q.z;
        currentAngle = calculateAngle(q);  // 再通过传感器获得一次倾角数据。
        Serial.println("OK2");
        if (currentAngle > Open_the_parachute_inclination) { // 通过第二次获得的倾角数据来重复上面的判断，防止他妈的传感器发抽风病输出有问题的数据导致误开伞，这里基本能把所有的噪波都过滤了。如果达到开伞倾角，则进入下面最后一步的判断。
            // Current_rocket_inclination = Filter_Current_Rocket_Inclination(1);
            if (currentAngle > Open_the_parachute_inclination) { // 第三次对当前的倾角进行判断，经过三次在不同时间点上对火箭倾角取值应该可以把所有的错误数据都过滤了，下面可以开伞了
            w_float = q.w;
            x_float = q.x;
            y_float = q.y;
            z_float = q.z;
            currentAngle = calculateAngle(q);
                digitalWrite(Parachute_ignition_switch, Connect);
                Serial.println("OK3");


                
                if (OPEN == 0) {
                    File dataFile = SD.open("data.txt", FILE_WRITE);
                    //  gpsRead();
                    Serial.println("OK4");
                    if (dataFile) {
                        dataFile.println("Opened");   // 在数据文件中写入开伞的标记点
                        dataFile.close();
                   Serial.println("OK5");
                      
  
                        
                    }
                    OPEN = 1;

                }

            }
        }
    }
  }
  wdt_reset();//喂狗
}


// int Filter_Current_Rocket_Inclination(int b)
// {
//   int temp;
//   temp=Get_Current_Rocket_Inclination(b);
//   if(temp==0)
//      {
//        return last_rocket_inclination;
//      }
//   else
//      {
//        last_rocket_inclination=temp;
//        return last_rocket_inclination;
//      }
// }
// /*上面的函数是过滤传感器传回的无效值,参数b可以为0,1,2,分别代表传感器的Z,X,Y三个轴,本程序中取X轴,如果用其它轴来判断,则部分程序需要相应的修改*/





// float Get_Current_Rocket_Inclination(int c)
// {
//     float rocket_inclination=0;
//     a1=mpu.getIntStatus();
//     a3=mpu.getFIFOCount();
//     if (a1&0x02)
//          {
//           while (a3<a2) a3=mpu.getFIFOCount();
//           mpu.getFIFOBytes(a4, a2);
//           a3-=a2;
//           mpu.resetFIFO();
//           mpu.dmpGetQuaternion(&a5, a4);
//           mpu.dmpGetGravity(&a6, &a5);
//           mpu.dmpGetYawPitchRoll(inclination, &a5, &a6);
//           rocket_inclination=inclination[c]*180/M_PI;
//           return rocket_inclination;
//          }
// }
/*上面的函数是通过传感器来获得当前X轴与地平线的倾角数据,由于不可知的原因,获得的数据可能有噪点,不能拿来直接使用,需要进行适当的过滤.参数c可以为0,1,2,分别代表传感器的Z,X,Y三个轴,本程序中取X轴,如果用其它轴来判断,则部分程序需要相应的修改*/      

void Init_MPU_6050()
{
    mpu.initialize();
    mpu.setFullScaleGyroRange(3);
    mpu.setFullScaleAccelRange(3);
    mpu.setDLPFMode(6);
    mpu.setDHPFMode(1);
    mpu.dmpInitialize();
    mpu.setDMPEnabled(true);
    a2 = mpu.dmpGetFIFOPacketSize();
}
/*上面的函数是对MPU6050进行初始化和一些必要的设置*/





void sendInt16(int16_t value, uint16_t &sumCheck, uint16_t &addCheck) {
    // 分解int16_t为两个byte并发送
    uint8_t lowByte = value & 0xFF; // 低位
    uint8_t highByte = value >> 8;  // 高位
    
    mySerial.write(lowByte);
    mySerial.write(highByte);
    
    // 更新和校验和附加校验
    sumCheck += lowByte; // 直接累加两个字节
    addCheck += sumCheck;           // 累加当前的和校验到附加校 
    sumCheck += highByte; // 直接累加两个字节
    addCheck += sumCheck;
}

void SendDualQuaternion() {
  byte HEAD = 0xAA;
  byte D_ADDR = 0xFF;
  // byte S_ADDR = 0xDC;
  // byte D_ADDR = 0xFE;
  byte ID = 0x04;
  int16_t LEN = 9;
  int16_t R4W = w_float * 10000; 
  int16_t R4X = x_float * 10000;  
  int16_t R4Y = y_float * 10000;  
  int16_t R4Z = z_float * 10000; 

  uint8_t MIX = 0x01;

  uint16_t sumCheck = 0;
  uint16_t addCheck = 0;

  // 发送头部并逐步更新校验
  mySerial.write(HEAD);
  sumCheck += HEAD;
  addCheck += sumCheck;
  
  // mySerial.write(S_ADDR);
  // sumCheck += S_ADDR;
  // addCheck += sumCheck;

  mySerial.write(D_ADDR);
  sumCheck += D_ADDR;
  addCheck += sumCheck;
  
  mySerial.write(ID);
  sumCheck += ID;
  addCheck += sumCheck;
  

  mySerial.write(LEN);
  sumCheck += LEN;
  addCheck += sumCheck;

  // sendInt16(LEN, sumCheck, addCheck);

  // 发送数据并更新校验
  sendInt16(R4W, sumCheck, addCheck);
  sendInt16(R4X, sumCheck, addCheck);
  sendInt16(R4Y, sumCheck, addCheck);
  sendInt16(R4Z, sumCheck, addCheck);
  
  mySerial.write(MIX);
  sumCheck += MIX;
  addCheck += sumCheck;
  
  // 确保只取低8位
  sumCheck &= 0xFF;
  addCheck &= 0xFF;

  // 发送校验值
  mySerial.write((uint8_t)sumCheck);
  mySerial.write((uint8_t)addCheck);
}


 
void sendHight() {    //通过协议向上位机发送高度数据
  byte HEAD = 0xAA;
  // byte S_ADDR = 0xFF;
  byte D_ADDR = 0xFF;
  byte ID = 0x05;
  int16_t LEN = 13;
  int32_t Hight = Aint;
  


  uint16_t sumCheck2 = 0;
  uint16_t addCheck2 = 0;

  // 发送头部并逐步更新校验
  mySerial.write(HEAD);
  sumCheck2 += HEAD;
  addCheck2 += sumCheck2;
  


  mySerial.write(D_ADDR);
  sumCheck2 += D_ADDR;
  addCheck2 += sumCheck2;
  
  mySerial.write(ID);
  sumCheck2 += ID;
  addCheck2 += sumCheck2;
  
  mySerial.write(LEN);
  sumCheck2 += LEN;
  addCheck2 += sumCheck2;

  // 发送数据并更新校验
  sendInt32(Hight, sumCheck2, addCheck2);
  sendInt32(0, sumCheck2, addCheck2);
  sendInt32(0, sumCheck2, addCheck2);
  mySerial.write(1);

  sumCheck2 += 1;
  addCheck2 += sumCheck2;
  
  // 确保只取低8位
  sumCheck2 &= 0xFF;
  addCheck2 &= 0xFF;

  // 发送校验值
  mySerial.write((uint8_t)sumCheck2);
  mySerial.write((uint8_t)addCheck2);
}

void sendInt32(int32_t value, uint16_t &sumCheck2, uint16_t &addCheck2) {
    // 分解int32_t为四个byte并发送
    uint8_t byte1 = value & 0xFF;          // 最低位
    uint8_t byte2 = (value >> 8) & 0xFF;   // 第二低位
    uint8_t byte3 = (value >> 16) & 0xFF;  // 第二高位
    uint8_t byte4 = (value >> 24) & 0xFF;  // 最高位
    
    mySerial.write(byte1);
    mySerial.write(byte2);
    mySerial.write(byte3);
    mySerial.write(byte4);
    
    // 更新和校验和附加校验
    updateChecksum(byte1, sumCheck2, addCheck2);
    updateChecksum(byte2, sumCheck2, addCheck2);
    updateChecksum(byte3, sumCheck2, addCheck2);
    updateChecksum(byte4, sumCheck2, addCheck2);
}

void updateChecksum(uint8_t byte, uint16_t &sumCheck, uint16_t &addCheck) {
    sumCheck += byte;
    addCheck += sumCheck;  // 累加当前的sumCheck值到addCheck
}





void SDwrite(float w_float, float x_float, float y_float, float z_float, 
             float Vertical_acceleration, float Vertical_accelerationY, float Vertical_accelerationZ, 
             int Aint) {
  File dataFile = SD.open("data.txt", FILE_WRITE);
  
  if (dataFile) {
    dataFile.print("S "); dataFile.print(" "); // 开始标志

    // 写入四元数数据
    dataFile.print(w_float, 5); dataFile.print(" ");
    dataFile.print(x_float, 5); dataFile.print(" ");
    dataFile.print(y_float, 5); dataFile.print(" ");
    dataFile.print(z_float, 5); dataFile.print(" ");

    // 写入加速度数据
    dataFile.print(Vertical_acceleration, 2); dataFile.print(" ");
    dataFile.print(Vertical_accelerationY, 2); dataFile.print(" ");
    dataFile.print(Vertical_accelerationZ, 2); dataFile.print(" ");
    

    // 写入高度数据
    dataFile.print(Aint, 1); dataFile.print(" ");

    // 结束标志 V，并在最后换行 
    dataFile.println("V");  

    // 关闭文件
    dataFile.close();
  } 
}

void clearSDCard() {
  File root = SD.open("/");
  if (root) {
    clearDirectory(root);
    root.close();
  }
}

void clearDirectory(File dir) {
  while (true) {
    File entry = dir.openNextFile();
    if (!entry) {
      // No more files
      break;
    }
    if (entry.isDirectory()) {
      clearDirectory(entry);
    } else {
      SD.remove(entry.name());
    }
    entry.close();
  }
  // 确保所有操作完成
  delay(100);
}

void ReadAndParseQuaternion() {//读取四元数并解析出俯仰角
  if (mpu.dmpGetCurrentFIFOPacket(fifoBuffer)) {
    mpu.dmpGetQuaternion(&q, fifoBuffer);   
    mpu.dmpGetGravity(&gravity, &q);   
    w_float = q.w;
    x_float = q.x;   
    y_float = q.y;   
    z_float = q.z;        
    currentAngle = calculateAngle(q);
  } 
}

void ReadHight(){
  char status;
  double T,P,p0,Altitude;
  status = pressure.startTemperature();
  status = pressure.getTemperature(T);
  status = pressure.startPressure(3);
  status = pressure.getPressure(P,T);
  p0 = pressure.sealevel(P,ALTITUDE);
  Altitude = pressure.altitude(P,p0);
  // Aint = static_cast<int16_t>((Altitude - Hight) * 100); 
  Aint = static_cast<int16_t>(Altitude * 100); 
}

