/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

/****************************************************************************/
/***                                                                      ***/
/***   (C) 2005 Dierk Ohlerich, all rights reserved                       ***/
/***                                                                      ***/
/****************************************************************************/

#define sPEDANTIC_WARN 1

#include "base/math.hpp"

#if !sCONFIG_OPTION_NOSYSTEM
#include "base/system.hpp"
#endif

/****************************************************************************/

sDontInitializeT sDontInitialize;

/****************************************************************************/

static void sInitPerlin();

static void sInitMath()
{
  sSetFloat();
  sInitPerlin();
}

#if !sCONFIG_OPTION_NOSYSTEM
sADDSUBSYSTEM(Math,0x18,sInitMath,0);
#endif

/****************************************************************************/
/****************************************************************************/

// Find real roots (=Nullstellen) of at� + bt + c.
int sSolveQuadratic(float t[],float a,float b,float c)
{
  if(a == 0.0f) // yes, exact compares everywhere!
  {
    if(b == 0.0f)
      return 0;

    t[0] = -c/b;
    return 1;
  }

  float discr = b*b - 4.0f*a*c;
  if(discr > 0.0f)
  {
    float x = -0.5f * (b + (b >= 0.0f ? 1.0f : -1.0f) * sSqrt(discr));
    t[0] = x / a;
    t[1] = c / x;
    return 2;
  }
  else if(discr < 0.0f)
    return 0;
  else
  {
    t[0] = -0.5f * b / a;
    return 1;
  }
}

/****************************************************************************/
/****************************************************************************/

double sRombergIntegral(sIntegrand f_,void *user,double a,double b,int maxOrder,double maxError)
{
  double t[20];
  sVERIFY(0 < maxOrder && maxOrder <= 20);
  sVERIFY(a <= b);
  
  if(a == b)
    return 0.0f;

  // first order: trapezoid rule over whole interval
  double result;
  double h = b-a;
  t[0] = result = 0.5 * h * (f_(a,user) + f_(b,user));
  
  // successively higher orders
  int i = 1;
  for(int n=1; i<maxOrder; i++, n *= 2, h *= 0.5)
  {
    // trapezoid rule for next-higher order
    double sum = 0.0;
    for(int j=0; j<n; j++)
      sum += f_(a + h*(j + 0.5),user);
    
    sum = 0.5 * (t[0] + h * sum);
    
    // richardson extrapolation
    double lastIn = t[0];
    double lastOut = sum;
    double ff = 4.0;
    t[0] = sum;
    
    for(int j=1; j<=i; j++, ff *= 4.0)
    {
      double in = t[j];
      t[j] = lastOut = (ff * lastOut - lastIn) / (ff - 1.0);
      lastIn = in;
    }
    
    // (relative) error estimation
    double errorEst = result ? (t[i] - result) / result : 0.0;
    result = t[i];

    if(-maxError < errorEst && errorEst < maxError) // don't ask.
      break;
  }
  
  return result;
}

/****************************************************************************/
/****************************************************************************/


/****************************************************************************/
/****************************************************************************/

void sVector30::InitColor(uint32_t col)
{
  x = ((col&0x00ff0000)>>16)/255.0f;
  y = ((col&0x0000ff00)>> 8)/255.0f;
  z = ((col&0x000000ff)    )/255.0f;
}

void sVector30::InitColor(uint32_t col,float amp)
{
  InitColor(col);
  x *= amp;
  y *= amp;
  z *= amp;
}

uint32_t sVector30::GetColor() const
{
  uint32_t col;

  col = (sClamp<int>(int(x*255+0.5f),0,255)<<16)
      | (sClamp<int>(int(y*255+0.5f),0,255)<< 8)
      | (sClamp<int>(int(z*255+0.5f),0,255)    )
      | 0x00000000;

  return col;
}

void sVector30::Cross(sVector30Arg a,sVector30Arg b)      
{
  *this = sVector30(a.y*b.z-a.z*b.y
                   ,a.z*b.x-a.x*b.z
                   ,a.x*b.y-a.y*b.x);
}

void sMatrix34::Fade(float f,const sMatrix34 &mat0,const sMatrix34 &mat1)
{
  i.Fade(f,mat0.i,mat1.i);
  j.Fade(f,mat0.j,mat1.j);
  k.Fade(f,mat0.k,mat1.k);
  l.Fade(f,mat0.l,mat1.l);
}

void sMatrix34::FadeOrthonormal(float f,const sMatrix34 &mat0,const sMatrix34 &mat1)
{
  i.Fade(f,mat0.i,mat1.i);
  k.Fade(f,mat0.k,mat1.k);
  l.Fade(f,mat0.l,mat1.l);
  k.Unit();
  j.Cross(k,i);
  j.Unit();
  i.Cross(j,k);
}

void sMatrix34::FadeOrthonormalPrecise(float f,const sMatrix34 &mat0,const sMatrix34 &mat1)
{
  i.Fade(f,mat0.i,mat1.i);
  k.Fade(f,mat0.k,mat1.k);
  l.Fade(f,mat0.l,mat1.l);
  k.UnitPrecise();
  j.Cross(k,i);
  j.UnitPrecise();
  i.Cross(j,k);
}

/****************************************************************************/
// Angle(): get the angle between two vectors (this and a)
/****************************************************************************/
float sVector30::Angle(sVector30Arg a) const
{
  float s = x*a.x+y*a.y+z*a.z;
  float n1 = x*x+y*y+z*z;
  float n2 = a.x*a.x+a.y*a.y+a.z*a.z;
  return sFACos(sClamp(s*sFRSqrt(n1*n2), -1.0f, 1.0f));
};

void sVector30::Rotate(const sMatrix34 &m,sVector30Arg v)
{
  x = v.x*m.i.x + v.y*m.j.x + v.z*m.k.x + m.l.x;
  y = v.x*m.i.y + v.y*m.j.y + v.z*m.k.y + m.l.y;
  z = v.x*m.i.z + v.y*m.j.z + v.z*m.k.z + m.l.z;
}

void sVector30::Unit()
{
  float e = 1.0f*x*x + 1.0f*y*y + 1.0f*z*z;

  if(e>1e-12f)
  {
    e=sFRSqrt(e);
    x = float(e*x);
    y = float(e*y);
    z = float(e*z);
  }
  else
  {
    Init(1,0,0);
  }
}

/****************************************************************************/

void sVector30::UnitPrecise()
{
  float e = 1.0f*x*x + 1.0f*y*y + 1.0f*z*z;

  if(e>1e-12f)
  {
    e=sRSqrt(e);
    x = float(e*x);
    y = float(e*y);
    z = float(e*z);
  }
  else
  {
    Init(1,0,0);
  }
}

/****************************************************************************/

void sVector31::InitColor(uint32_t col)
{
  x = ((col&0x00ff0000)>>16)/255.0f;
  y = ((col&0x0000ff00)>> 8)/255.0f;
  z = ((col&0x000000ff)    )/255.0f;
}

void sVector31::InitColor(uint32_t col,float amp)
{
  InitColor(col);
  x *= amp;
  y *= amp;
  z *= amp;
}

uint32_t sVector31::GetColor() const
{
  uint32_t col;

  col = (sClamp<int>(int(x*255+0.5f),0,255)<<16)
      | (sClamp<int>(int(y*255+0.5f),0,255)<< 8)
      | (sClamp<int>(int(z*255+0.5f),0,255)    )
      | 0xff000000;

  return col;
}

/****************************************************************************/

void sVector4::InitColor(uint32_t col)
{
  x = ((col&0x00ff0000)>>16)/255.0f;
  y = ((col&0x0000ff00)>> 8)/255.0f;
  z = ((col&0x000000ff)    )/255.0f;
  w = ((col&0xff000000)>>24)/255.0f;
}

void sVector4::InitColor(uint32_t col,float amp)
{
  InitColor(col);
  x *= amp;
  y *= amp;
  z *= amp;
  w *= amp;
}

uint32_t sVector4::GetColor() const
{
  uint32_t col;

  col = (sClamp<int>(int(x*255+0.5f),0,255)<<16)
      | (sClamp<int>(int(y*255+0.5f),0,255)<< 8)
      | (sClamp<int>(int(z*255+0.5f),0,255)    )
      | (sClamp<int>(int(w*255+0.5f),0,255)<<24);

  return col;
}

void sVector4::InitMRGB8(uint32_t col)
{
  w = ((col&0xff000000)>>24)+1.0f;
  x = w*((col&0x00ff0000)>>16)/255.0f;
  y = w*((col&0x0000ff00)>> 8)/255.0f;
  z = w*((col&0x000000ff)    )/255.0f;
  w = 1.0f;
}

uint32_t sVector4::GetMRGB8() const
{
  float max = sFFloor(sMax(sMax(x,y),z))+1.0f;

  uint32_t col =  (sClamp<int>(int(x/max*255),0,255)<<16)
            | (sClamp<int>(int(y/max*255),0,255)<< 8)
            | (sClamp<int>(int(z/max*255),0,255)    )
            | (sClamp<int>(int(max-1.0f),0,255)<<24);
  return col;
}

void sVector4::InitMRGB16(uint64_t col)
{
  w =   ((col>>48)&0xffff)+1.0f;
  x = w*((col>>32)&0xffff)/65536.0f;
  y = w*((col>>16)&0xffff)/65536.0f;
  z = w*((col    )&0xffff)/65536.0f;
  w = 1.0f;
}

uint64_t sVector4::GetMRGB16() const
{
  float max = sFFloor(sMax(sMax(x,y),z))+1.0f;

  uint64_t col =  (sClamp<int64_t>(int64_t(x/max*65536.0f),0,65536)<<32)
            | (sClamp<int64_t>(int64_t(y/max*65536.0f),0,65536)<<16)
            | (sClamp<int64_t>(int64_t(z/max*65536.0f),0,65536)    )
            | (sClamp<int64_t>(int64_t(max-1.0f),0,65536)<<48);
  return col;
}

void sVector4::InitPlane(sVector31Arg pos,sVector30Arg norm)
{
  x = norm.x;
  y = norm.y;
  z = norm.z;
  w = - (pos ^ norm);
}

void sVector4::InitPlane(sVector31Arg p0,sVector31Arg p1,sVector31Arg p2)
{
  sVector30 norm;
  norm.Cross(p0-p1,p0-p2);
  norm.Unit();
  x = norm.x;
  y = norm.y;
  z = norm.z;
  w = - (p0 ^ norm);
}

sBool sVector4::operator==(sVector4Arg v) const
{
  return  x == v.x &&
          y == v.y &&
          z == v.z &&
          w == v.w;
}


void sVector4::Unit4Precise()
{
  float e = 1.0f*x*x + 1.0f*y*y + 1.0f*z*z + 1.0f*w*w;

  if(e>1e-12f)
  {
    e=sRSqrt(e);
    x = float(e*x);
    y = float(e*y);
    z = float(e*z);
    w = float(e*w);
  }
  else
  {
    Init(1,0,0,0);
  }
}

void sVector4::Unit4()
{
  float e = 1.0f*x*x + 1.0f*y*y + 1.0f*z*z + 1.0f*w*w;

  if(e>1e-12f)
  {
    e=sFRSqrt(e);
    x = float(e*x);
    y = float(e*y);
    z = float(e*z);
    w = float(e*w);
  }
  else
  {
    Init(1,0,0,0);
  }
}

/****************************************************************************/


sMatrix34::sMatrix34(const sMatrix34CM &m)
{
  m.ConvertTo(*this);
}

sMatrix34 &sMatrix34::operator=(const sMatrix34CM &m)
{
  m.ConvertTo(*this);
  return *this;
}

void sMatrix34::Init()
{
  i.Init(1,0,0);
  j.Init(0,1,0);
  k.Init(0,0,1);
  l.Init(0,0,0);
}

void sMatrix34::Euler(float x,float y,float z)
{
  EulerXYZ(x,y,z); // it's the same!
}

void sMatrix34::EulerXYZ(float x,float y,float z)
{
  float sx,sy,sz;
  float cx,cy,cz;

  sFSinCos(x,sx,cx);
  sFSinCos(y,sy,cy);
  sFSinCos(z,sz,cz);

  i.x = cy*cz;
  i.y = cy*sz;
  i.z = -sy;
  j.x = sx*cz*sy - cx*sz;
  j.y = sx*sz*sy + cx*cz;
  j.z = sx*cy;
  k.x = cx*cz*sy + sx*sz;
  k.y = cx*sz*sy - sx*cz;
  k.z = cx*cy;
}


void sMatrix34::FindEulerXYZ(float &x,float &y,float &z) const
{
  float rx,ry,rz;
  float sy = -i.z;

  if(sy >= 0.99999f) // ry very close to 90� (singular)
  {
    ry = sPIF / 2.0f;
    rx = sATan2(j.x,j.y);
    rz = 0.0f;
  }
  else if(sy <= -0.99999f) // ry very close to -90� (singular)
  {
    ry = -sPIF / 2.0f;
    rx = sATan2(-j.x,-k.x);
    rz = 0.0f;
  }
  else
  {
    float cy = sFSqrt(j.z*j.z + k.z*k.z);  // this can be positive or negative!
    ry = sATan2(sy,cy);
    rz = sATan2(i.y,i.x);
    rx = sATan2(j.z,k.z);
  }

  x = rx;
  y = ry;
  z = rz;
}

// this chooses a minimal z rotation. this is the better choice for interactive cameras
// cy can be positive or negative! that is where the problem comes from.

void sMatrix34::FindEulerXYZ2(float &x,float &y,float &z) const
{
  float rx,ry,rz;
  float sy = -i.z;

  if(sy >= 0.99999f) // ry very close to 90� (singular)
  {
    ry = sPIF / 2.0f;
    rx = sATan2(j.x,j.y);
    rz = 0.0f;
  }
  else if(sy <= -0.99999f) // ry very close to -90� (singular)
  {
    ry = -sPIF / 2.0f;
    rx = sATan2(-j.x,-k.x);
    rz = 0.0f;
  }
  else
  {
    float rzp = sATan2(i.y,i.x);
    float rzn = sATan2(-i.y,-i.x);
    float cy = sFSqrt(j.z*j.z + k.z*k.z);    // evil square root that has two possible outcomes!
//    if(i.y>=0)
    if(sFAbs(rzp)<sFAbs(rzn))               // CHAOS: ask dierk before changing!
    {
      rx = sATan2(j.z,k.z);
      ry = sATan2(sy,cy);
      rz = rzp;//sATan2(i.y,i.x);
    }
    else
    {
      rx = sATan2(-j.z,-k.z);
      ry = sATan2(sy,-cy);
      rz = rzn;//sATan2(-i.y,-i.x);
    }
  }

  x = rx;
  y = ry;
  z = rz;
}


void sMatrix34::CubeFace(int n)
{
  static const float dir[6][3] =
  {
    { 1, 0, 0},       // sTCF_POSX
    {-1, 0, 0},       // sTCF_NEGX
    { 0, 1, 0},       // sTCF_POSY
    { 0,-1 ,0},       // sTCF_NEGY
    { 0, 0, 1},       // sTCF_POSZ
    { 0, 0,-1},       // sTCF_NEGZ
  };
  static const float up[6][3] =
  {
    { 0, 1, 0},      // sTCF_POSX
    { 0, 1, 0},      // sTCF_NEGX
    { 0, 0,-1},      // sTCF_POSY
    { 0, 0, 1},      // sTCF_NEGY
    { 0, 1, 0},      // sTCF_POSZ
    { 0, 1, 0},      // sTCF_NEGZ
  };
  
  j = sVector30(up [n][0], up [n][1], up [n][2]);
  k = sVector30(dir[n][0], dir[n][1], dir[n][2]);
  i.Cross(j,k);
  l.Init(0,0,0);
}

void sMatrix34::Look(sVector30Arg v)
{
  j.Init(0,1,0);
  k = v;
  k.Unit();
  i.Cross(j,k);
  i.Unit();         // ist dieses unit �berfl�ssig ?
  j.Cross(k,i);
  j.Unit();
  l.Init(0,0,0);
}

//sTODO("vasco: replace sMatrix34::Look by sMatrix34::Look_");

void sMatrix34::Look(sVector30Arg dir, sVector30Arg up)
{
  k = dir;
  k.Unit();
  j = up;
  j.Unit();
  i.Cross(j,k);
  i.Unit();
  j.Cross(k,i);
  l.Init(0,0,0);
}

void sMatrix34::Look_(sVector30Arg dir, sVector30Arg up)
{
  j = up;
  k = dir;
  k.Unit();
  i.Cross(j,k);
  i.Unit();
  j.Cross(k,i);
  j.Unit();
  l.Init(0,0,0);
}

void sMatrix34::LookPrecise(sVector30Arg v)
{
  j.Init(0,1,0);
  k = v;
  k.UnitPrecise();
  i.Cross(j,k);
  i.UnitPrecise();         // ist dieses unit �berfl�ssig ?
  j.Cross(k,i);
  j.UnitPrecise();
  l.Init(0,0,0);
}

void sMatrix34::LookPrecise(sVector30Arg dir, sVector30Arg up)
{
  k = dir;
  k.UnitPrecise();
  j = up;
  j.UnitPrecise();
  i.Cross(j,k);
  i.UnitPrecise();
  j.Cross(k,i);
  l.Init(0,0,0);
}

void sMatrix34::LookAt(sVector31Arg dest,sVector31Arg pos)
{
  sVector30 dir = dest-pos;
  sVector31 l_ = pos;
  dir.Unit();
  Look(dir);
  l = l_;
}

void sMatrix34::LookAt(sVector31Arg dest, sVector31Arg pos, sVector30Arg up)
{
  sVector30 dir = dest-pos;
  sVector31 l_ = pos;
  dir.Unit();
  Look(dir,up);
  l = l_;
}


void sMatrix34::RotateAxis(sVector30Arg v,float a)
{
  sVector30 u;
  float as,ac;

  u = v;
  u.Unit();

  ac = float(sFCos(a));
  as = float(sFSin(a));

  i.x = (1-u.x*u.x)*ac + u.x*u.x + 0;
  i.y = ( -u.x*u.y)*ac + u.x*u.y - u.z*as;
  i.z = ( -u.x*u.z)*ac + u.x*u.z + u.y*as;
  j.x = ( -u.y*u.x)*ac + u.y*u.x + u.z*as;
  j.y = (1-u.y*u.y)*ac + u.y*u.y + 0;
  j.z = ( -u.y*u.z)*ac + u.y*u.z - u.x*as;
  k.x = ( -u.z*u.x)*ac + u.z*u.x - u.y*as;
  k.y = ( -u.z*u.y)*ac + u.z*u.y + u.x*as;
  k.z = (1-u.z*u.z)*ac + u.z*u.z + 0;

  // as the euler functions, L will not be initialized!
}

void sMatrix34::Cross(sVector30Arg v)
{
  i.Init(   0, v.z,-v.y);
  j.Init(-v.z,   0, v.x);
  k.Init( v.y,-v.x,   0);
  l.Init(   0,   0,   0);
}

void sMatrix34::Trans3()
{
  sSwap(i.y,j.x);
  sSwap(i.z,k.x);
  sSwap(j.z,k.y);
}

void sMatrix34::TransR()
{
  sVector31 v,w;

  Trans3();

  v.Init(-l.x,-l.y,-l.z);
  l.Init(0,0,0);

  w = v*(*this);

  l = w;
}

void sMatrix34::InvertOrthogonal()
{
  float x,y,z;

  x = sFInvSqrt(i^i);
  y = sFInvSqrt(j^j);
  z = sFInvSqrt(k^k);
  i *= x;
  j *= y;
  k *= z;
  Trans3();
  i *= x;
  j *= y;
  k *= z;

  l = sVector31(sVector30(-l)*(*this));
}

float sMatrix34::Determinant3x3() const
{
  return (i.x*j.y*k.z + i.y*j.z*k.x + i.z*j.x*k.y)
       - (i.z*j.y*k.x + i.x*j.z*k.y + i.y*j.x*k.z);
}

sBool sMatrix34::Invert3()
{
  // Invert a 3x3 using cofactors.  This is faster than using a generic
  // Gaussian elimination because of the loop overhead of such a method.
  sMatrix34 inv;

  inv.i.x = j.y*k.z - j.z*k.y;
  inv.i.y = i.z*k.y - i.y*k.z;
  inv.i.z = i.y*j.z - i.z*j.y;
  inv.j.x = j.z*k.x - j.x*k.z;
  inv.j.y = i.x*k.z - i.z*k.x;
  inv.j.z = i.z*j.x - i.x*j.z;
  inv.k.x = j.x*k.y - j.y*k.x;
  inv.k.y = i.y*k.x - i.x*k.y;
  inv.k.z = i.x*j.y - i.y*j.x;

  float det = i.x*inv.i.x + i.y*inv.j.x + i.z*inv.k.x;
  if (sFAbs(det)<sEPSILON) return sFALSE;
  det=1.0f/det;
  i=inv.i*det;
  j=inv.j*det;
  k=inv.k*det;
  return sTRUE;
}

sBool sMatrix34::Invert34()
{
  if (!Invert3()) return sFALSE;
  l = sVector31(sVector30(-l)*(*this));
  return sTRUE;
}

void sMatrix34::Orthonormalize ()
{
  // Gram-Schmidt orthogonalization
  i.Unit();
  j-=i*(i^j);
  j.Unit();
  k-=i*(i^k)+j*(j^k);
  k.Unit();
}

void sMatrix34::Init(sQuaternionArg q, sVector31Arg p)
{
  float xx = 2.0f*q.i*q.i;
  float xy = 2.0f*q.i*q.j;
  float xz = 2.0f*q.i*q.k;

  float yy = 2.0f*q.j*q.j;
  float yz = 2.0f*q.j*q.k;
  float zz = 2.0f*q.k*q.k;

  float xw = 2.0f*q.i*q.r;
  float yw = 2.0f*q.j*q.r;
  float zw = 2.0f*q.k*q.r;

  i.x = 1.0f - (yy + zz);
  j.x =        (xy + zw);
  k.x =        (xz - yw);

  i.y =        (xy - zw);
  j.y = 1.0f - (xx + zz);
  k.y =        (yz + xw);

  i.z =        (xz + yw);
  j.z =        (yz - xw);
  k.z = 1.0f - (xx + yy);

  l=p;
}

void sMatrix34::Init(sQuaternionArg q)
{
  float xx = 2.0f*q.i*q.i;
  float xy = 2.0f*q.i*q.j;
  float xz = 2.0f*q.i*q.k;

  float yy = 2.0f*q.j*q.j;
  float yz = 2.0f*q.j*q.k;
  float zz = 2.0f*q.k*q.k;

  float xw = 2.0f*q.i*q.r;
  float yw = 2.0f*q.j*q.r;
  float zw = 2.0f*q.k*q.r;

  i.x = 1.0f - (yy + zz);
  j.x =        (xy + zw);
  k.x =        (xz - yw);

  i.y =        (xy - zw);
  j.y = 1.0f - (xx + zz);
  k.y =        (yz + xw);

  i.z =        (xz + yw);
  j.z =        (yz - xw);
  k.z = 1.0f - (xx + yy);

  l.Init(0,0,0);
}

void sMatrix34::Scale(float x, float y, float z)
{
  i.x *= x;
  i.y *= x;
  i.z *= x;
  j.x *= y;
  j.y *= y;
  j.z *= y;
  k.x *= z;
  k.y *= z;
  k.z *= z;
}

void sMatrix34::ThreePoint(const sVector31 &p0,const sVector31 &p1,const sVector31 &p2,const sVector30 &tweak)
{
  sVector30 db;

  k = p2-p0;
  k.Unit();
  db = p1-p0;
  db.Unit();

  i.Cross(db,k); 
  i = i + tweak;
  j.Cross(k,i); 
  i.Unit();
  j.Unit(); 
}

void sMatrix34::ThreePoint_(const sVector31 &p0,const sVector31 &p1,const sVector31 &p2,const sVector30 &tweak)
{
  sVector30 db;

  k = p2-p0;
  k.Unit();
  db = p1-p0;
  db.Unit();

  j.Cross(k,db); 
  j = j + tweak;
  j.Unit(); 
  i.Cross(j,k); 
  i.Unit();
}

void sMatrix34::RotateAroundPivot(sQuaternionArg rot,sVector31Arg pivot)
{
  // translate by -pivot, rotate, translate by pivot
  // => total transform: Rot*(x-pivot) + pivot = Rot*x + (pivot - Rot*pivot)
  Init(rot);
  l = pivot - (sVector30(pivot) * *this);
}

void sMatrix34::RotateAroundPivot(sVector30Arg axis,float angle,sVector31Arg pivot)
{
  // same as above, with axis-angle rotation
  RotateAxis(axis,angle);
  l = pivot - (sVector30(pivot) * *this);
}

/****************************************************************************/

void sMatrix34CM::Ident()
{
  x.Init(1.0f,0.0f,0.0f,0.0f);
  y.Init(0.0f,1.0f,0.0f,0.0f);
  z.Init(0.0f,0.0f,1.0f,0.0f);
}

sMatrix34CM::sMatrix34CM(const sMatrix34 &m)
{
  *this = m;
}

sMatrix34CM &sMatrix34CM::operator=(const sMatrix34 &m)
{
  x.x = m.i.x;
  x.y = m.j.x;
  x.z = m.k.x;
  x.w = m.l.x;
  y.x = m.i.y;
  y.y = m.j.y;
  y.z = m.k.y;
  y.w = m.l.y;
  z.x = m.i.z;
  z.y = m.j.z;
  z.z = m.k.z;
  z.w = m.l.z;

  return *this;
}

void sMatrix34CM::ConvertTo(sMatrix34& s)const
{
  s.i.x = x.x;
  s.j.x = x.y;
  s.k.x = x.z;
  s.l.x = x.w;
  s.i.y = y.x;
  s.j.y = y.y;
  s.k.y = y.z;
  s.l.y = y.w;
  s.i.z = z.x;
  s.j.z = z.y;
  s.k.z = z.z;
  s.l.z = z.w;
}

float sMatrix34CM::Determinant3x3() const
{
  return (x.x*y.y*z.z + y.x*z.y*x.z + z.x*x.y*y.z)
       - (z.x*y.y*x.z + x.x*z.y*y.z + y.x*x.y*z.z);
}

/****************************************************************************/

void sMatrix44::Init()
{
  i.Init(1.0f, 0.0f, 0.0f, 0.0f);
  j.Init(0.0f, 1.0f, 0.0f, 0.0f);
  k.Init(0.0f, 0.0f, 1.0f, 0.0f);
  l.Init(0.0f, 0.0f, 0.0f, 1.0f);
}

void sMatrix44::Trans4()
{
  sSwap(i.y,j.x);
  sSwap(i.z,k.x);
  sSwap(i.w,l.x);
  sSwap(j.z,k.y);
  sSwap(j.w,l.y);
  sSwap(k.w,l.z);
}

void sMatrix44::Trans4(const sMatrix44 &mat)
{
  i.x = mat.i.x;
  i.y = mat.j.x;
  i.z = mat.k.x;
  i.w = mat.l.x;

  j.x = mat.i.y;
  j.y = mat.j.y;
  j.z = mat.k.y;
  j.w = mat.l.y;

  k.x = mat.i.z;
  k.y = mat.j.z;
  k.z = mat.k.z;
  k.w = mat.l.z;

  l.x = mat.i.w;
  l.y = mat.j.w;
  l.z = mat.k.w;
  l.w = mat.l.w;
}

void sMatrix44::Scale(float x, float y, float z)
{
  i.x *= x;
  j.y *= y;
  k.z *= z;
}

void sMatrix44::Orthonormalize ()
{
  // Gram-Schmidt orthogonalization
  i.Unit4();
  j-=i*(i^j);
  j.Unit4();
  k-=i*(i^k)+j*(j^k);
  k.Unit4();
  l-=i*(i^l)+j*(j^l)+k*(k^l);
  l.Unit4();
}

sBool sMatrix44::operator==(const sMatrix44 &mat) const
{
  return  i == mat.i &&
          j == mat.j &&
          k == mat.k &&
          l == mat.l;
}

void sMatrix44::Invert(const sMatrix44 &a)
{
  float idet = 1.0f/a.Determinant();

  i.x = (a.j.z*a.k.w*a.l.y - a.j.w*a.k.z*a.l.y + a.j.w*a.k.y*a.l.z - a.j.y*a.k.w*a.l.z - a.j.z*a.k.y*a.l.w + a.j.y*a.k.z*a.l.w)*idet;
  i.y = (a.i.w*a.k.z*a.l.y - a.i.z*a.k.w*a.l.y - a.i.w*a.k.y*a.l.z + a.i.y*a.k.w*a.l.z + a.i.z*a.k.y*a.l.w - a.i.y*a.k.z*a.l.w)*idet;
  i.z = (a.i.z*a.j.w*a.l.y - a.i.w*a.j.z*a.l.y + a.i.w*a.j.y*a.l.z - a.i.y*a.j.w*a.l.z - a.i.z*a.j.y*a.l.w + a.i.y*a.j.z*a.l.w)*idet;
  i.w = (a.i.w*a.j.z*a.k.y - a.i.z*a.j.w*a.k.y - a.i.w*a.j.y*a.k.z + a.i.y*a.j.w*a.k.z + a.i.z*a.j.y*a.k.w - a.i.y*a.j.z*a.k.w)*idet;
  j.x = (a.j.w*a.k.z*a.l.x - a.j.z*a.k.w*a.l.x - a.j.w*a.k.x*a.l.z + a.j.x*a.k.w*a.l.z + a.j.z*a.k.x*a.l.w - a.j.x*a.k.z*a.l.w)*idet;
  j.y = (a.i.z*a.k.w*a.l.x - a.i.w*a.k.z*a.l.x + a.i.w*a.k.x*a.l.z - a.i.x*a.k.w*a.l.z - a.i.z*a.k.x*a.l.w + a.i.x*a.k.z*a.l.w)*idet;
  j.z = (a.i.w*a.j.z*a.l.x - a.i.z*a.j.w*a.l.x - a.i.w*a.j.x*a.l.z + a.i.x*a.j.w*a.l.z + a.i.z*a.j.x*a.l.w - a.i.x*a.j.z*a.l.w)*idet;
  j.w = (a.i.z*a.j.w*a.k.x - a.i.w*a.j.z*a.k.x + a.i.w*a.j.x*a.k.z - a.i.x*a.j.w*a.k.z - a.i.z*a.j.x*a.k.w + a.i.x*a.j.z*a.k.w)*idet;
  k.x = (a.j.y*a.k.w*a.l.x - a.j.w*a.k.y*a.l.x + a.j.w*a.k.x*a.l.y - a.j.x*a.k.w*a.l.y - a.j.y*a.k.x*a.l.w + a.j.x*a.k.y*a.l.w)*idet;
  k.y = (a.i.w*a.k.y*a.l.x - a.i.y*a.k.w*a.l.x - a.i.w*a.k.x*a.l.y + a.i.x*a.k.w*a.l.y + a.i.y*a.k.x*a.l.w - a.i.x*a.k.y*a.l.w)*idet;
  k.z = (a.i.y*a.j.w*a.l.x - a.i.w*a.j.y*a.l.x + a.i.w*a.j.x*a.l.y - a.i.x*a.j.w*a.l.y - a.i.y*a.j.x*a.l.w + a.i.x*a.j.y*a.l.w)*idet;
  k.w = (a.i.w*a.j.y*a.k.x - a.i.y*a.j.w*a.k.x - a.i.w*a.j.x*a.k.y + a.i.x*a.j.w*a.k.y + a.i.y*a.j.x*a.k.w - a.i.x*a.j.y*a.k.w)*idet;
  l.x = (a.j.z*a.k.y*a.l.x - a.j.y*a.k.z*a.l.x - a.j.z*a.k.x*a.l.y + a.j.x*a.k.z*a.l.y + a.j.y*a.k.x*a.l.z - a.j.x*a.k.y*a.l.z)*idet;
  l.y = (a.i.y*a.k.z*a.l.x - a.i.z*a.k.y*a.l.x + a.i.z*a.k.x*a.l.y - a.i.x*a.k.z*a.l.y - a.i.y*a.k.x*a.l.z + a.i.x*a.k.y*a.l.z)*idet;
  l.z = (a.i.z*a.j.y*a.l.x - a.i.y*a.j.z*a.l.x - a.i.z*a.j.x*a.l.y + a.i.x*a.j.z*a.l.y + a.i.y*a.j.x*a.l.z - a.i.x*a.j.y*a.l.z)*idet;
  l.w = (a.i.y*a.j.z*a.k.x - a.i.z*a.j.y*a.k.x + a.i.z*a.j.x*a.k.y - a.i.x*a.j.z*a.k.y - a.i.y*a.j.x*a.k.z + a.i.x*a.j.y*a.k.z)*idet;
}


float sMatrix44::Determinant() const
{
  return i.w*j.z*k.y*l.x - i.z*j.w*k.y*l.x - i.w*j.y*k.z*l.x + i.y*j.w*k.z*l.x
       + i.z*j.y*k.w*l.x - i.y*j.z*k.w*l.x - i.w*j.z*k.x*l.y + i.z*j.w*k.x*l.y
       + i.w*j.x*k.z*l.y - i.x*j.w*k.z*l.y - i.z*j.x*k.w*l.y + i.x*j.z*k.w*l.y
       + i.w*j.y*k.x*l.z - i.y*j.w*k.x*l.z - i.w*j.x*k.y*l.z + i.x*j.w*k.y*l.z
       + i.y*j.x*k.w*l.z - i.x*j.y*k.w*l.z - i.z*j.y*k.x*l.w + i.y*j.z*k.x*l.w
       + i.z*j.x*k.y*l.w - i.x*j.z*k.y*l.w - i.y*j.x*k.z*l.w + i.x*j.y*k.z*l.w;
}

void sMatrix44::Perspective(float left, float right, float top, float bottom, float front, float back)
{
  // see D3DXMatrixPerspectiveOffCenterLH

  float xx = 2.0f * front / (right - left);
  float yy = 2.0f * front / (top - bottom);
  float xz = (left + right) / (left - right);
  float yz = (top + bottom) / (bottom - top);

  float zz = back / (back - front);
  float zw = front*back / (front - back);
  
  float wz = 1.0f;

  i.x = xx;
  i.y = 0.0f;
  i.z = 0.0f;
  i.w = 0.0f;

  j.x = 0.0f;
  j.y = yy;
  j.z = 0.0f;
  j.w = 0.0f;

  k.x = xz;
  k.y = yz;
  k.z = zz;
  k.w = wz;

  l.x = 0.0f;
  l.y = 0.0f;
  l.z = zw;
  l.w = 0.0f;
}


/****************************************************************************/

#ifndef sHASINTRINSIC_MATRIXMUL

sMatrix44 operator* (const sMatrix44 &a,const sMatrix44 &b)
{
  sMatrix44 r;

  r.i.x = a.i.x*b.i.x + a.i.y*b.j.x + a.i.z*b.k.x + a.i.w*b.l.x;
  r.i.y = a.i.x*b.i.y + a.i.y*b.j.y + a.i.z*b.k.y + a.i.w*b.l.y;
  r.i.z = a.i.x*b.i.z + a.i.y*b.j.z + a.i.z*b.k.z + a.i.w*b.l.z;
  r.i.w = a.i.x*b.i.w + a.i.y*b.j.w + a.i.z*b.k.w + a.i.w*b.l.w;

  r.j.x = a.j.x*b.i.x + a.j.y*b.j.x + a.j.z*b.k.x + a.j.w*b.l.x;
  r.j.y = a.j.x*b.i.y + a.j.y*b.j.y + a.j.z*b.k.y + a.j.w*b.l.y;
  r.j.z = a.j.x*b.i.z + a.j.y*b.j.z + a.j.z*b.k.z + a.j.w*b.l.z;
  r.j.w = a.j.x*b.i.w + a.j.y*b.j.w + a.j.z*b.k.w + a.j.w*b.l.w;

  r.k.x = a.k.x*b.i.x + a.k.y*b.j.x + a.k.z*b.k.x + a.k.w*b.l.x;
  r.k.y = a.k.x*b.i.y + a.k.y*b.j.y + a.k.z*b.k.y + a.k.w*b.l.y;
  r.k.z = a.k.x*b.i.z + a.k.y*b.j.z + a.k.z*b.k.z + a.k.w*b.l.z;
  r.k.w = a.k.x*b.i.w + a.k.y*b.j.w + a.k.z*b.k.w + a.k.w*b.l.w;

  r.l.x = a.l.x*b.i.x + a.l.y*b.j.x + a.l.z*b.k.x + a.l.w*b.l.x;
  r.l.y = a.l.x*b.i.y + a.l.y*b.j.y + a.l.z*b.k.y + a.l.w*b.l.y;
  r.l.z = a.l.x*b.i.z + a.l.y*b.j.z + a.l.z*b.k.z + a.l.w*b.l.z;
  r.l.w = a.l.x*b.i.w + a.l.y*b.j.w + a.l.z*b.k.w + a.l.w*b.l.w;
  
  return r;
}


sMatrix34 operator* (const sMatrix34 &a,const sMatrix34 &b)
{
  sMatrix34 r;

  r.i.x = a.i.x*b.i.x + a.i.y*b.j.x + a.i.z*b.k.x;
  r.i.y = a.i.x*b.i.y + a.i.y*b.j.y + a.i.z*b.k.y;
  r.i.z = a.i.x*b.i.z + a.i.y*b.j.z + a.i.z*b.k.z;

  r.j.x = a.j.x*b.i.x + a.j.y*b.j.x + a.j.z*b.k.x;
  r.j.y = a.j.x*b.i.y + a.j.y*b.j.y + a.j.z*b.k.y;
  r.j.z = a.j.x*b.i.z + a.j.y*b.j.z + a.j.z*b.k.z;

  r.k.x = a.k.x*b.i.x + a.k.y*b.j.x + a.k.z*b.k.x;
  r.k.y = a.k.x*b.i.y + a.k.y*b.j.y + a.k.z*b.k.y;
  r.k.z = a.k.x*b.i.z + a.k.y*b.j.z + a.k.z*b.k.z;

  r.l.x = a.l.x*b.i.x + a.l.y*b.j.x + a.l.z*b.k.x + b.l.x;
  r.l.y = a.l.x*b.i.y + a.l.y*b.j.y + a.l.z*b.k.y + b.l.y;
  r.l.z = a.l.x*b.i.z + a.l.y*b.j.z + a.l.z*b.k.z + b.l.z;
  
  return r;
}

sMatrix34CM operator* (const sMatrix34CM &a,const sMatrix34CM &b)
{
  sMatrix34CM r;

  r.x.x = a.x.x*b.x.x + a.y.x*b.x.y + a.z.x*b.x.z;
  r.y.x = a.x.x*b.y.x + a.y.x*b.y.y + a.z.x*b.y.z;
  r.z.x = a.x.x*b.z.x + a.y.x*b.z.y + a.z.x*b.z.z;

  r.x.y = a.x.y*b.x.x + a.y.y*b.x.y + a.z.y*b.x.z;
  r.y.y = a.x.y*b.y.x + a.y.y*b.y.y + a.z.y*b.y.z;
  r.z.y = a.x.y*b.z.x + a.y.y*b.z.y + a.z.y*b.z.z;

  r.x.z = a.x.z*b.x.x + a.y.z*b.x.y + a.z.z*b.x.z;
  r.y.z = a.x.z*b.y.x + a.y.z*b.y.y + a.z.z*b.y.z;
  r.z.z = a.x.z*b.z.x + a.y.z*b.z.y + a.z.z*b.z.z;

  r.x.w = a.x.w*b.x.x + a.y.w*b.x.y + a.z.w*b.x.z + b.x.w;
  r.y.w = a.x.w*b.y.x + a.y.w*b.y.y + a.z.w*b.y.z + b.y.w;
  r.z.w = a.x.w*b.z.x + a.y.w*b.z.y + a.z.w*b.z.z + b.z.w;
  
  return r;
}

#endif

/****************************************************************************/

sMatrix34 operator* (const sMatrix34 &a,float b)
{
  sMatrix34 res;
  res.i=a.i*b;
  res.j=a.j*b;
  res.k=a.k*b;
  res.l=sVector31(a.l*b);
  return res;
}

sMatrix44 operator* (const sMatrix44 &a,float b)
{
  sMatrix44 res;
  res.i=a.i*b;
  res.j=a.j*b;
  res.k=a.k*b;
  res.l=a.l*b;
  return res;
}

sMatrix34& operator *= (sMatrix34 &a, float b)
{
  a.i*=b;
  a.j*=b;
  a.k*=b;
  a.l=sVector31(a.l*b);
  return a;
}

sMatrix44& operator *= (sMatrix44 &a, float b)
{
  a.i*=b;
  a.j*=b;
  a.k*=b;
  a.l*=b;
  return a;
}

sMatrix34 operator+ (const sMatrix34 &a, const sMatrix34 &b)
{
  sMatrix34 res;
  res.i=a.i+b.i;
  res.j=a.i+b.j;
  res.k=a.k+b.k;
  res.l=a.l+sVector30(b.l);
  return res;
}

sMatrix44 operator+ (const sMatrix44 &a, const sMatrix44 &b)
{
  sMatrix44 res;
  res.i=a.i+b.i;
  res.j=a.i+b.j;
  res.k=a.k+b.k;
  res.l=a.l+b.l;
  return res;
}

sMatrix34 operator- (const sMatrix34 &a, const sMatrix34 &b)
{
  sMatrix34 res;
  res.i=a.i-b.i;
  res.j=a.i-b.j;
  res.k=a.k-b.k;
  res.l=a.l-sVector30(b.l);
  return res;
}

sMatrix44 operator- (const sMatrix44 &a, const sMatrix44 &b)
{
  sMatrix44 res;
  res.i=a.i-b.i;
  res.j=a.i-b.j;
  res.k=a.k-b.k;
  res.l=a.l-b.l;
  return res;
}

sMatrix34& operator+= (sMatrix34 &a, const sMatrix34 &b)
{
  a.i+=b.i;
  a.j+=b.j;
  a.k+=b.k;
  a.l+=sVector30(b.l);
  return a;
}

sMatrix44& operator+= (sMatrix44 &a, const sMatrix44 &b)
{
  a.i+=b.i;
  a.j+=b.j;
  a.k+=b.k;
  a.l+=b.l;
  return a;
}

sMatrix34& operator-= (sMatrix34 &a, const sMatrix34 &b)
{
  a.i-=b.i;
  a.j-=b.j;
  a.k-=b.k;
  a.l-=sVector30(b.l);
  return a;
}

sMatrix44& operator-= (sMatrix44 &a, const sMatrix44 &b)
{
  a.i-=b.i;
  a.j-=b.j;
  a.k-=b.k;
  a.l-=b.l;
  return a;
}

/****************************************************************************/

#ifndef sHASINTRINSIC_VECTORMATRIXMUL

sVector30 operator* (sVector30Arg a,const sMatrix34 &b)
{
  return sVector30(
    a.x*b.i.x + a.y*b.j.x + a.z*b.k.x,
    a.x*b.i.y + a.y*b.j.y + a.z*b.k.y,
    a.x*b.i.z + a.y*b.j.z + a.z*b.k.z
  );
}

sVector31 operator* (sVector31Arg a,const sMatrix34 &b)
{
  return sVector31(
    a.x*b.i.x + a.y*b.j.x + a.z*b.k.x + b.l.x,
    a.x*b.i.y + a.y*b.j.y + a.z*b.k.y + b.l.y,
    a.x*b.i.z + a.y*b.j.z + a.z*b.k.z + b.l.z
  );
}

sVector4  operator* (sVector4Arg a,const sMatrix34 &b)
{
  return sVector4(
    a.x*b.i.x + a.y*b.j.x + a.z*b.k.x + a.w*b.l.x,
    a.x*b.i.y + a.y*b.j.y + a.z*b.k.y + a.w*b.l.y,
    a.x*b.i.z + a.y*b.j.z + a.z*b.k.z + a.w*b.l.z,
    a.w
  );
}


sVector30 operator* (sVector30Arg a,const sMatrix34CM &b)
{
  return sVector30(
    a^b.x,
    a^b.y,
    a^b.z
  );
}

sVector31 operator* (sVector31Arg a,const sMatrix34CM &b)
{
  return sVector31(
    a^b.x,
    a^b.y,
    a^b.z
  );
}

sVector4  operator* (sVector4Arg a,const sMatrix34CM &b)
{
  return sVector4(
    a^b.x,
    a^b.y,
    a^b.z,
    a.w
  );
}

sVector4  operator* (sVector30Arg a,const sMatrix44 &b)
{
  return sVector4(
    a.x*b.i.x + a.y*b.j.x + a.z*b.k.x,
    a.x*b.i.y + a.y*b.j.y + a.z*b.k.y,
    a.x*b.i.z + a.y*b.j.z + a.z*b.k.z,
    a.x*b.i.w + a.y*b.j.w + a.z*b.k.w
  );
}

sVector4  operator* (sVector31Arg a,const sMatrix44 &b)
{
  return sVector4(
    a.x*b.i.x + a.y*b.j.x + a.z*b.k.x + b.l.x,
    a.x*b.i.y + a.y*b.j.y + a.z*b.k.y + b.l.y,
    a.x*b.i.z + a.y*b.j.z + a.z*b.k.z + b.l.z,
    a.x*b.i.w + a.y*b.j.w + a.z*b.k.w + b.l.w
  );
}

sVector4  operator* (sVector4Arg a,const sMatrix44 &b)
{
  return sVector4(
    a.x*b.i.x + a.y*b.j.x + a.z*b.k.x + a.w*b.l.x,
    a.x*b.i.y + a.y*b.j.y + a.z*b.k.y + a.w*b.l.y,
    a.x*b.i.z + a.y*b.j.z + a.z*b.k.z + a.w*b.l.z,
    a.x*b.i.w + a.y*b.j.w + a.z*b.k.w + a.w*b.l.w
  );
}

#endif

sFormatStringBuffer& operator% (sFormatStringBuffer &f,sVector2Arg v)
{
  if(!*f.Format)
    return f;

  sFormatStringInfo info;
  f.GetInfo(info);

  f.Print(L"[");
  f.PrintFloat(info,v.x);
  f.Print(L",");
  f.PrintFloat(info,v.y);
  f.Print(L"]");
  f.Fill();

  return f;
}

sFormatStringBuffer& operator% (sFormatStringBuffer &f,const sVector30 &v)
{
  if (!*f.Format)
    return f;

  sFormatStringInfo info;
  f.GetInfo(info);

  f.Print(L"[");
  f.PrintFloat(info,v.x);
  f.Print(L",");
  f.PrintFloat(info,v.y);
  f.Print(L",");
  f.PrintFloat(info,v.z);
  f.Print(L",0]");
  f.Fill();

  return f;
}

sFormatStringBuffer& operator% (sFormatStringBuffer &f,const sVector31 &v)
{
  if (!*f.Format)
    return f;

  sFormatStringInfo info;
  f.GetInfo(info);

  f.Print(L"[");
  f.PrintFloat(info,v.x);
  f.Print(L",");
  f.PrintFloat(info,v.y);
  f.Print(L",");
  f.PrintFloat(info,v.z);
  f.Print(L",1]");
  f.Fill();

  return f;
}

sFormatStringBuffer& operator% (sFormatStringBuffer &f,const sVector4 &v)
{
  if (!*f.Format)
    return f;

  sFormatStringInfo info;
  f.GetInfo(info);

  f.Print(L"[");
  f.PrintFloat(info,v.x);
  f.Print(L",");
  f.PrintFloat(info,v.y);
  f.Print(L",");
  f.PrintFloat(info,v.z);
  f.Print(L",");
  f.PrintFloat(info,v.w);
  f.Print(L"]");
  f.Fill();

  return f;
}


sFormatStringBuffer& operator% (sFormatStringBuffer &f,const sMatrix34 &mat)
{
  if (!*f.Format)
    return f;

  sFormatStringInfo info;
  f.GetInfo(info);

  f.Print(L"i[");
  f.PrintFloat(info,mat.i.x);
  f.Print(L",");
  f.PrintFloat(info,mat.i.y);
  f.Print(L",");
  f.PrintFloat(info,mat.i.z);
  f.Print(L",0]\n");

  f.Print(L"j[");
  f.PrintFloat(info,mat.j.x);
  f.Print(L",");
  f.PrintFloat(info,mat.j.y);
  f.Print(L",");
  f.PrintFloat(info,mat.j.z);
  f.Print(L",0]\n");

  f.Print(L"k[");
  f.PrintFloat(info,mat.k.x);
  f.Print(L",");
  f.PrintFloat(info,mat.k.y);
  f.Print(L",");
  f.PrintFloat(info,mat.k.z);
  f.Print(L",0]\n");

  f.Print(L"l[");
  f.PrintFloat(info,mat.l.x);
  f.Print(L",");
  f.PrintFloat(info,mat.l.y);
  f.Print(L",");
  f.PrintFloat(info,mat.l.z);
  f.Print(L",1]\n");

  f.Fill();

  return f;
}


sFormatStringBuffer& operator% (sFormatStringBuffer &f,const sMatrix34CM &mat)
{
  if (!*f.Format)
    return f;

  sFormatStringInfo info;
  f.GetInfo(info);

  f.Print(L"a[");
  f.PrintFloat(info,mat.x.x);
  f.Print(L",");
  f.PrintFloat(info,mat.x.y);
  f.Print(L",");
  f.PrintFloat(info,mat.x.z);
  f.Print(L",");
  f.PrintFloat(info,mat.x.w);
  f.Print(L"]\n");

  f.Print(L"b[");
  f.PrintFloat(info,mat.y.x);
  f.Print(L",");
  f.PrintFloat(info,mat.y.y);
  f.Print(L",");
  f.PrintFloat(info,mat.y.z);
  f.Print(L",");
  f.PrintFloat(info,mat.y.w);
  f.Print(L"]\n");

  f.Print(L"c[");
  f.PrintFloat(info,mat.z.x);
  f.Print(L",");
  f.PrintFloat(info,mat.z.y);
  f.Print(L",");
  f.PrintFloat(info,mat.z.z);
  f.Print(L",");
  f.PrintFloat(info,mat.z.w);
  f.Print(L"]\n");


  f.Fill();

  return f;
}

sFormatStringBuffer& operator% (sFormatStringBuffer &f,const sMatrix44 &mat)
{
  if (!*f.Format)
    return f;

  sFormatStringInfo info;
  f.GetInfo(info);

  f.Print(L"i[");
  f.PrintFloat(info,mat.i.x);
  f.Print(L",");
  f.PrintFloat(info,mat.i.y);
  f.Print(L",");
  f.PrintFloat(info,mat.i.z);
  f.Print(L",");
  f.PrintFloat(info,mat.i.w);
  f.Print(L"]\n");

  f.Print(L"j[");
  f.PrintFloat(info,mat.j.x);
  f.Print(L",");
  f.PrintFloat(info,mat.j.y);
  f.Print(L",");
  f.PrintFloat(info,mat.j.z);
  f.Print(L",");
  f.PrintFloat(info,mat.j.w);
  f.Print(L"]\n");

  f.Print(L"k[");
  f.PrintFloat(info,mat.k.x);
  f.Print(L",");
  f.PrintFloat(info,mat.k.y);
  f.Print(L",");
  f.PrintFloat(info,mat.k.z);
  f.Print(L",");
  f.PrintFloat(info,mat.k.w);
  f.Print(L"]\n");

  f.Print(L"l[");
  f.PrintFloat(info,mat.l.x);
  f.Print(L",");
  f.PrintFloat(info,mat.l.y);
  f.Print(L",");
  f.PrintFloat(info,mat.l.z);
  f.Print(L",");
  f.PrintFloat(info,mat.l.w);
  f.Print(L"]\n");

  f.Fill();

  return f;
}

/****************************************************************************/
/***                                                                      ***/
/***   Quaternion                                                         ***/
/***                                                                      ***/
/****************************************************************************/

void sQuaternion::Unit()
{
  float e = sFRSqrt(1.0f*i*i + 1.0f*j*j + 1.0f*k*k + 1.0f*r*r);

  i = float(e*i);
  j = float(e*j);
  k = float(e*k);
  r = float(e*r);

  // no safety check, zero-quaternions should never come across
}

void sQuaternion::Lerp(float fade,sQuaternionArg a,sQuaternionArg b)
{
  float dot = a.r*b.r + a.i*b.i + a.j*b.j + a.k*b.k;
#if 1		// this part introduces the interpolation error over 90 DEG (in some cases)
  if(dot<0.f)
  {
    r = -a.r + (b.r+a.r)*fade;
    i = -a.i + (b.i+a.i)*fade;
    j = -a.j + (b.j+a.j)*fade;
    k = -a.k + (b.k+a.k)*fade;
  }
  else
#endif
  {
    r = a.r + (b.r-a.r)*fade;
    i = a.i + (b.i-a.i)*fade;
    j = a.j + (b.j-a.j)*fade;
    k = a.k + (b.k-a.k)*fade;
  }
}


void sQuaternion::Slerp(float fade,sQuaternionArg a,sQuaternionArg b)
{
  float f0,f1;
	float angle,s;
	float dot;

  dot = a.r*b.r + a.i*b.i + a.j*b.j + a.k*b.k;

  if(dot<=-1)     angle = sPIF;
  else if(dot>=1) angle = 0;
  else            angle = sFACos(dot);

	s=sFSin(angle);
	if(s==0)
  {
    *this = (angle<sPIF/2) ? a:b;
  }
  else
  {
	  f0 = float(sFSin((1-fade)*angle)/s);
	  f1 = float(sFSin(   fade *angle)/s);
	  r = f0*a.r + f1*b.r;
	  i = f0*a.i + f1*b.i;
	  j = f0*a.j + f1*b.j;
	  k = f0*a.k + f1*b.k;
  }

}


void sQuaternion::FastSlerp(float fade, sQuaternionArg a, sQuaternionArg b)
{
  // ideas taken from Jonathan Blow's paper "Hacking Quaternions" [Blow2002]
  // use a cubic spline to correct the speed error introduced by lerp
  // the function's error is bigger in the 0.5-1.0 range, so just
  // mirror the interpolation for the upper half.

  // special-case: no blend needed. so save the time
  if(fade<=0.f+0.00000001f)
  { i=a.i;
    j=a.j;
    k=a.k;
    r=a.r;
    return;
  } else  if(fade>=1.f-0.00000001f)
  { i=b.i;
    j=b.j;
    k=b.k;
    r=b.r;
    return;
  }

  float K = 1.0f - 0.8228677f * dot(a,b);
  K = K*K*0.5069269f;
  if (fade>=0.5f)
  { fade = 1.0f-fade;
    fade = 1.0f-((fade * K * (2*fade-3) + K + 1)*fade);
  } else
  { fade = (fade * K * (2*fade-3) + K + 1)*fade;
  }

  Lerp(fade,a,b);
#if 1
  float e = i*i + j*j + k*k + r*r;
	if(e<0.00000000001f)	// make the code bulletproof...
	{ i = j = r = 0;   k = 1;
		return;
	}
	e = sFRSqrt(e);
	i = e*i;
  j = e*j;
  k = e*k;
  r = e*r;
#endif
}

void sQuaternion::Init(const sMatrix34 &mat)
{
  float tr,s;
	
	tr = mat.i.x + mat.j.y + mat.k.z;

	if(tr>=0) 
  {
		s = (float)sFSqrt(tr + 1);
		r = s*0.5f;
		s = 0.5f / s;
		i = (mat.k.y - mat.j.z) * s;
		j = (mat.i.z - mat.k.x) * s;
		k = (mat.j.x - mat.i.y) * s;
	}
	else 
  {
		int index;
		if (mat.j.y > mat.i.x)
    {      
		  if (mat.k.z > mat.j.y) index=2; else index=1;
    }
    else
    {      
		  if (mat.k.z > mat.i.x) index=2; else index=0;
    }

		switch(index)
    {
		case 0:
			s = (float)sFSqrt((mat.i.x - (mat.j.y+mat.k.z)) + 1.0f );
			i = s*0.5f;
			s = 0.5f / s;
			j = (mat.i.y + mat.j.x) * s;
			k = (mat.k.x + mat.i.z) * s;
			r = (mat.k.y - mat.j.z) * s;
			break;
		case 1:
			s = (float)sFSqrt( (mat.j.y - (mat.k.z+mat.i.x)) + 1.0f );
			j = s*0.5f;
			s = 0.5f / s;
			k = (mat.j.z + mat.k.y) * s;
			i = (mat.i.y + mat.j.x) * s;
			r = (mat.i.z - mat.k.x) * s;
			break;
		case 2:
			s = (float)sFSqrt( (mat.k.z - (mat.i.x+mat.j.y)) + 1.0f );
			k = s*0.5f;
			s = 0.5f / s;
			i = (mat.k.x + mat.i.z) * s;
			j = (mat.j.z + mat.k.y) * s;
			r = (mat.j.x - mat.i.y) * s;
			break;
		}
	}
}

void sQuaternion::Init(sVector30Arg axis, float angle)
{
  float si;
  sFSinCos(angle/2,si,r);
  i=si*axis.x;
  j=si*axis.y;
  k=si*axis.z;
}

/****************************************************************************/

void sQuaternion::Euler(float h, float p, float b)
{
  float sh,ch,sp,cp,sb,cb;
  sFSinCos(h*0.5f,sh,ch);
  sFSinCos(p*0.5f,sp,cp);
  sFSinCos(b*0.5f,sb,cb);
  const float chcp=ch*cp;
  const float shsp=sh*sp;
  const float shcp=sh*cp;
  const float chsp=ch*sp;
  r = chcp*cb - shsp*sb;
  i = chcp*sb + shsp*cb;
  j = shcp*cb + chsp*sb;
  k = chsp*cb - shcp*sb;
}

sVector30 sQuaternion::GetEuler() const
{
  float h,p,b;
  const float d = i*j+k*r;
  if (d>0.4999f)
  {
    h=2.0f*sFATan2(i,r);
    p=sPIF/2.0f;
    b=0;
  }
  else if (d<-0.4999f)
  {
    h=-2.0f*sFATan2(i,r);
    p=-sPIF/2.0f;
    b=0;
  }
  else
  {
    const float i2=i*i;
    const float j2=j*j;
    const float k2=k*k;
    h=sFATan2(2.0f*(j*r-i*k),1.0f-2.0f*(j2-k2));
    p=sFASin(2.0f*d);
    b=sFATan2(2.0f*(i*r-j*k),1.0f-2.0f*(i2-k2));
  }
  return sVector30(h,p,b);
}


void sQuaternion::GetAxisAngle(sVector30 &axis, float &angle)
{
  float lensq=i*i+j*j+k*k;
  if (lensq>sEPSILON)
  {
    lensq=sFRSqrt(lensq);
    axis.Init(i*lensq,j*lensq,k*lensq);
    angle=2*sFACos(r);
  }
  else
  {
    axis.Init(1,0,0);
    angle=0;
  }
}


void sQuaternion::MakeRotation(sVector30Arg v1, sVector30Arg v2)
{
  sVector30 axis; 
  axis.Cross(v2,v1);
  float dot=v1^v2;
  if (dot < sEPSILON-1) 
  {
    r=i=k=0;
    j=1;
    return;
  }

  i=axis.x;
  j=axis.y;
  k=axis.z;
  r=dot+1;
  Unit();
}

void sQuaternion::Invert()
{
  float lensq=r*r+i*i+j*j+k*k;
  if (lensq>sEPSILON)
  {
    lensq=sFRSqrt(lensq);
    r*=lensq;
    i*=-lensq;
    j*=-lensq;
    k*=-lensq;
  }
}

void sQuaternion::Invert(sQuaternionArg q)
{
  float lensq=q.r*q.r+q.i*q.i+q.j*q.j+q.k*q.k;
  if (lensq>sEPSILON)
  {
    lensq=sFRSqrt(lensq);
    r=lensq*q.r;
    i=-lensq*q.i;
    j=-lensq*q.j;
    k=-lensq*q.k;
  }
}

void sQuaternion::InvertNorm()
{  
  i=-i;
  j=-j;
  k=-k;
}

  /*

Quaternion fast_simple_rotation(const Vector3 &a, const Vector3 &b) {
    Vector3 axis = cross_product(a, b);
    float dot = dot_product(a, b);
    if (dot < -1.0f + DOT_EPSILON) return Quaternion(0, 1, 0, 0);
 
    Quaternion result(axis.x * 0.5f, axis.y * 0.5f, axis.z * 0.5f, (dot + 1.0f) * 0.5f);
    fast_normalize(&result);
 
    return result;
}

  public void set(Quat4d q1) {
	double test = q1.x*q1.y + q1.z*q1.w;
	if (test > 0.499) { // singularity at north pole
		heading = 2 * atan2(q1.x,q1.w);
		attitude = Math.PI/2;
		bank = 0;
		return;
	}
	if (test < -0.499) { // singularity at south pole
		heading = -2 * atan2(q1.x,q1.w);
		attitude = - Math.PI/2;
		bank = 0;
		return;
	}
    double sqx = q1.x*q1.x;
    double sqy = q1.y*q1.y;
    double sqz = q1.z*q1.z;
    heading = atan2(2*q1.y*q1.w-2*q1.x*q1.z , 1 - 2*sqy - 2*sqz);
	attitude = asin(2*test);
	bank = atan2(2*q1.x*q1.w-2*q1.y*q1.z , 1 - 2*sqx - 2*sqz)
}
*/

/*
public final void rotate(double heading, double attitude, double bank) {
    // Assuming the angles are in radians.
    double c1 = Math.cos(heading/2);
    double s1 = Math.sin(heading/2);
    double c2 = Math.cos(attitude/2);
    double s2 = Math.sin(attitude/2);
    double c3 = Math.cos(bank/2);
    double s3 = Math.sin(bank/2);
    double c1c2 = c1*c2;
    double s1s2 = s1*s2;
    w =c1c2*c3 - s1s2*s3;
  	x =c1c2*s3 + s1s2*c3;
	y =s1*c2*c3 + c1*s2*s3;
	z =c1*s2*c3 - s1*c2*s3;
  }
  */

/****************************************************************************/

sQuaternion operator* (sQuaternionArg a,sQuaternionArg b)
{
  sQuaternion q;

  q.i = a.i*b.r + a.r*b.i - a.k*b.j + a.j*b.k;
  q.j = a.j*b.r + a.k*b.i + a.r*b.j - a.i*b.k;
  q.k = a.k*b.r - a.j*b.i + a.i*b.j + a.r*b.k;
  q.r = a.r*b.r - a.i*b.i - a.j*b.j - a.k*b.k;

  return q;
}

sVector30 operator* (sVector30Arg a,sQuaternionArg b)
{
  /*
    From terrainshader: (I derived this by hand and optimized it for min. PS instruction count)
    q=quaternion, v=vector

    float3 t0 = cross(q.xyz,v.xyz);
    float4 t1 = q+q;

    return v + t1.w*t0 + cross(t1.xyz,t0);
    
    In this case the imaginary part is inverted, so you need to switch the order for both cross products.
  */
  
  // cross of vector part of v with q
  float t0x = a.y*b.k - a.z*b.j;
  float t0y = a.z*b.i - a.x*b.k;
  float t0z = a.x*b.j - a.y*b.i;

  // the sum
  float t1x = b.i + b.i;
  float t1y = b.j + b.j;
  float t1z = b.k + b.k;
  float t1r = b.r + b.r;

  // final expression
  return sVector30(
    a.x + t1r * t0x + t0y*t1z - t0z*t1y,
    a.y + t1r * t0y + t0z*t1x - t0x*t1z,
    a.z + t1r * t0z + t0x*t1y - t0y*t1x
  );
}

sVector31 operator* (sVector31Arg a,sQuaternionArg b)
{
  // same as above.

  // cross of vector part of v with q
  float t0x = a.y*b.k - a.z*b.j;
  float t0y = a.z*b.i - a.x*b.k;
  float t0z = a.x*b.j - a.y*b.i;

  // the sum
  float t1x = b.i + b.i;
  float t1y = b.j + b.j;
  float t1z = b.k + b.k;
  float t1r = b.r + b.r;

  // final expression
  return sVector31(
    a.x + t1r * t0x + t0y*t1z - t0z*t1y,
    a.y + t1r * t0y + t0z*t1x - t0x*t1z,
    a.z + t1r * t0z + t0x*t1y - t0y*t1x
  );
}

sFormatStringBuffer& operator% (sFormatStringBuffer &f, sQuaternionArg q)
{
  if (!*f.Format)
    return f;

  sFormatStringInfo info;
  f.GetInfo(info);

  f.Print(L"[");
  f.PrintFloat(info,q.r);
  f.Print(L"|");
  f.PrintFloat(info,q.i);
  f.Print(L",");
  f.PrintFloat(info,q.j);
  f.Print(L",");
  f.PrintFloat(info,q.k);
  f.Print(L"]");
  f.Fill();

  return f;
}

/****************************************************************************/
/***                                                                      ***/
/***   more vector math                                                   ***/
/***                                                                      ***/
/****************************************************************************/

sAABBox::sAABBox()
{
  Clear();
}

void sAABBox::Clear()
{
  Min.x = 1e30f;
  Min.y = 1e30f;
  Min.z = 1e30f;
  Max.x = -1e30f;
  Max.y = -1e30f;
  Max.z = -1e30f;
}

void sAABBox::Init(const sAABBoxC &s)
{
  Min = s.Center-s.Radius;
  Max = s.Center+s.Radius;
}

sBool sAABBox::HitPoint(sVector31Arg p) const
{
  return (p.x>=Min.x && p.x<=Max.x &&
          p.y>=Min.y && p.y<=Max.y && 
          p.z>=Min.z && p.z<=Max.z);
}

void sAABBox::And(const sAABBox &box)
{
  if(box.Min.x>Min.x) Min.x = box.Min.x;
  if(box.Min.y>Min.y) Min.y = box.Min.y;
  if(box.Min.z>Min.z) Min.z = box.Min.z;
  if(box.Max.x<Max.x) Max.x = box.Max.x;
  if(box.Max.y<Max.y) Max.y = box.Max.y;
  if(box.Max.z<Max.z) Max.z = box.Max.z;
}

void sAABBox::Add(const sAABBox &box)
{
  if(box.Min.x<Min.x) Min.x = box.Min.x;
  if(box.Min.y<Min.y) Min.y = box.Min.y;
  if(box.Min.z<Min.z) Min.z = box.Min.z;
  if(box.Max.x>Max.x) Max.x = box.Max.x;
  if(box.Max.y>Max.y) Max.y = box.Max.y;
  if(box.Max.z>Max.z) Max.z = box.Max.z;
}

void sAABBox::Add(sVector31Arg p)
{
  if(p.x<Min.x) Min.x = p.x;
  if(p.y<Min.y) Min.y = p.y;
  if(p.z<Min.z) Min.z = p.z;
  if(p.x>Max.x) Max.x = p.x;
  if(p.y>Max.y) Max.y = p.y;
  if(p.z>Max.z) Max.z = p.z;
}

void sAABBox::Add(const sAABBox &box,const sMatrix34 &mat)
{
  sVector31 v[8];
  box.MakePoints(v);
  for(int i=0;i<8;i++)
    Add(v[i]*mat);
}

void sAABBox::Add(const sAABBoxC &box,const sMatrix34CM &mat)
{
  sAABBox b;
  sMatrix34 m(mat);
  b.Init(box);
  Add(b,m);
}

void sAABBox::MakePoints(sVector31 *v) const
{
  v[0].Init(Min.x,Min.y,Min.z);
  v[1].Init(Max.x,Min.y,Min.z);
  v[2].Init(Max.x,Max.y,Min.z);
  v[3].Init(Min.x,Max.y,Min.z);
  v[4].Init(Min.x,Min.y,Max.z);
  v[5].Init(Max.x,Min.y,Max.z);
  v[6].Init(Max.x,Max.y,Max.z);
  v[7].Init(Min.x,Max.y,Max.z);
}

sBool sAABBox::HitRay(float &dist,const sRay &ray) const
{
  float min = 0.0f, max = 1e+20f;

  for(int i=0;i<3;i++)
  {
    if(ray.Dir[i])
    {
      float inv = 1.0f / ray.Dir[i];
      float t0 = (Min[i] - ray.Start[i]) * inv;
      float t1 = (Max[i] - ray.Start[i]) * inv;
      if(t0 > t1) sSwap(t0,t1);
      min = sMax(min,t0);
      max = sMin(max,t1);
      if(min > max) return sFALSE;
    }
    else if(ray.Start[i] < Min[i] || ray.Start[i] > Max[i])
      return sFALSE;
  }

  dist = min;
  return sTRUE;
}

// this is *almost* the same as sIntersectRayAABB, but it handles the case where one or more
// components of dir are zero differently (=without using IEEE infinities). 
sBool sAABBox::HitInvRay(float &dist,sVector31Arg origin,sVector30Arg invDir) const
{
  float min = 0.0f, max = 1e+20f;

  // x and z before y since we have lots of tests in +y or -y direction
  if(invDir.x)
  {
    float t0 = (Min.x - origin.x) * invDir.x;
    float t1 = (Max.x - origin.x) * invDir.x;
    float tMin = sMin(t0,t1);
    float tMax = sMax(t0,t1);
    min = sMax(min,tMin);
    max = sMin(max,tMax);
  }
  else if(origin.x < Min.x || origin.x > Max.x)
    return sFALSE;

  if(invDir.z)
  {
    float t0 = (Min.z - origin.z) * invDir.z;
    float t1 = (Max.z - origin.z) * invDir.z;
    float tMin = sMin(t0,t1);
    float tMax = sMax(t0,t1);
    min = sMax(min,tMin);
    max = sMin(max,tMax);
  }
  else if(origin.z < Min.z || origin.z > Max.z)
    return sFALSE;

  if(invDir.y)
  {
    float t0 = (Min.y - origin.y) * invDir.y;
    float t1 = (Max.y - origin.y) * invDir.y;
    float tMin = sMin(t0,t1);
    float tMax = sMax(t0,t1);
    min = sMax(min,tMin);
    max = sMin(max,tMax);
  }
  else if(origin.y < Min.y || origin.y > Max.y)
    return sFALSE;

  if(min > max)
    return sFALSE;

  dist = min;
  return sTRUE;

  //for(int i=0;i<3;i++)
  //{
  //  if(invDir[i])
  //  {
  //    float t0 = (Min[i] - origin[i]) * invDir[i];
  //    float t1 = (Max[i] - origin[i]) * invDir[i];
  //    if(t0 > t1) sSwap(t0,t1);
  //    min = sMax(min,t0);
  //    max = sMin(max,t1);
  //    if(min > max) return sFALSE;
  //  }
  //  else if(origin[i] < Min[i] || origin[i] > Max[i])
  //    return sFALSE;
  //}
  //
  //dist = min;
  //return sTRUE;
}

float sAABBox::CalcArea()const
{
  float area = (Max.x-Min.x)*(Max.y-Min.y);
  area += (Max.y-Min.y)*(Max.z-Min.z);
  area += (Max.z-Min.z)*(Max.x-Min.x);
  return area*2.0f;
}

float sAABBox::CalcVolume()const
{
  float area = (Max.x-Min.x)*(Max.y-Min.y)*(Max.z-Min.z);
  return area;
}

sBool sAABBox::IsEmpty() const
{
  return (Min.x>=Max.x || Min.y>=Max.y || Min.z>=Max.z);
}


sBool sAABBox::IsValid() const
{
  return (Min.x<=Max.x || Min.y<=Max.y || Min.z<=Max.z);
}

sBool sAABBox::Intersects(const sAABBox &b) const
{
  return sMax(Min.x,b.Min.x) <= sMin(Max.x,b.Max.x)
    && sMax(Min.y,b.Min.y) <= sMin(Max.y,b.Max.y)
    && sMax(Min.z,b.Min.z) <= sMin(Max.z,b.Max.z);
}

sBool sAABBox::IntersectsXZ(const sAABBox &b) const
{
  return sMax(Min.x,b.Min.x) <= sMin(Max.x,b.Max.x) && sMax(Min.z,b.Min.z) <= sMin(Max.z,b.Max.z);
}

sBool sAABBox::IntersectsMovingBox(const sAABBox &box,sVector30Arg v,float tMin,float tMax) const
{
  sVector30 invV;
  invV.x = v.x ? 1.0f / v.x : 0.0f;
  invV.y = v.y ? 1.0f / v.y : 0.0f;
  invV.z = v.z ? 1.0f / v.z : 0.0f;
  return IntersectsMovingBoxInv(box,invV,tMin,tMax);
}

sBool sAABBox::IntersectsMovingBoxInv(const sAABBox &box,sVector30Arg invV,float tMin,float tMax) const
{
  if(tMin > tMax)
    return sFALSE;

  for(int i=0;i<3;i++)
  {
    float iv = invV[i];

    if(iv)
    {
      // calc start/end time of interval overlap
      float t0 = (Min[i] - box.Max[i]) * iv;
      float t1 = (Max[i] - box.Min[i]) * iv;
      if(iv < 0.0f)
        sSwap(t0,t1);

      // intersect with current interval
      tMin = sMax(tMin,t0);
      tMax = sMin(tMax,t1);
      if(tMin > tMax) // intersection empty?
        return sFALSE;
    }
    else if(Max[i] < box.Min[i] || Min[i] > box.Max[i]) // stationary along that axis, check for overlap
      return sFALSE;
  }

  return sTRUE;
}

float sAABBox::DistanceToSq(sVector31Arg p) const
{
  float d=0.0f;
  
  if(p.x < Min.x)       d += sSquare(p.x-Min.x);
  else if(p.x > Max.x)  d += sSquare(p.x-Max.x);

  if(p.y < Min.y)       d += sSquare(p.y-Min.y);
  else if(p.y > Max.y)  d += sSquare(p.y-Max.y);

  if(p.z < Min.z)       d += sSquare(p.z-Min.z);
  else if(p.z > Max.z)  d += sSquare(p.z-Max.z);

  return d;
}

sAABBox &sAABBox::operator*=(const sMatrix34 &m)
{
  sVector31 v[8];
  MakePoints(v);

  for(int i=0;i<8;i++)
    v[i] = v[i]*m;
  Clear();
  for(int i=0;i<8;i++)
    Add(v[i]);

  return *this;
}

int sAABBox::Classify(sVector30Arg n, float d)
{
  sVector31 c = Center();
  float radius = Size().Length() * 0.5f;

  return ((n ^ c) - d) < radius;
}

sFormatStringBuffer& operator% (sFormatStringBuffer &f,const sAABBox &bbox)
{
  if (!*f.Format)
    return f;

  sFormatStringInfo info;
  f.GetInfo(info);

  f.Print(L"min[");
  f.PrintFloat(info,bbox.Min.x);
  f.Print(L",");
  f.PrintFloat(info,bbox.Min.y);
  f.Print(L",");
  f.PrintFloat(info,bbox.Min.z);
  f.Print(L"]");
  f.Print(L"-max[");
  f.PrintFloat(info,bbox.Max.x);
  f.Print(L",");
  f.PrintFloat(info,bbox.Max.y);
  f.Print(L",");
  f.PrintFloat(info,bbox.Max.z);
  f.Print(L"]");
  f.Fill();

  return f;
}

/****************************************************************************/
/****************************************************************************/

sBool sRay::HitTriangle(float &dist,sVector31Arg p0,sVector31Arg p1,sVector31Arg p2) const
{
  sVector30 d1,d2,n,org,org1,org2;
  float angle,e1,e2;
  float b;
  
  org = Start-p0;
  d2 = p1-p0;
  d1 = p2-p0;
  n.Cross(d1,d2);

  angle = Dir^n;
  if(angle<=sEPSILON)
    return 0;

  org2.Cross(org,d2); 
  e2 = Dir^org2; 
  if(e2<0) return 0;

  org1.Cross(d1,org);
  e1 = Dir^org1;
  if(e1<0 || e1+e2>angle) return 0;

  b = -org^n; 
  if(b<0)return 0;

  dist = b/angle;
  return 1;
}

sBool sRay::HitTriangleDoubleSided(float &dist,sVector31Arg p0,sVector31Arg p1,sVector31Arg p2) const
{
  sVector30 d1,d2,n,org,org1,org2;
  float angle,e1,e2;
  float b;
  
  org = Start-p0;
  d2 = p1-p0;
  d1 = p2-p0;
  n.Cross(d1,d2);

  angle = Dir^n;
  if(angle<0)
  {
    angle = -angle;
    n.Neg();
    sSwap(d1,d2);
  }
  if(angle<=sEPSILON)
    return 0;

  org2.Cross(org,d2); 
  e2 = Dir^org2; 
  if(e2<=0) return 0;

  org1.Cross(d1,org);
  e1 = Dir^org1;
  if(e1<=0 || e1+e2>angle) return 0;

  b = -org^n; 
  if(b<=0)return 0;

  dist = b/angle;
  return 1;
}

sBool sRay::HitTriangleBary(float &dist,sVector31Arg p0,sVector31Arg p1,sVector31Arg p2,float &u0,float &u1,float &u2) const
{
  sVector30 d1,d2,n,org,org1,org2;
  float angle,e1,e2;
  float b;

  org = Start-p0;
  d2 = p1-p0;
  d1 = p2-p0;
  n.Cross(d1,d2);

  angle = Dir^n;
  if(angle<=sEPSILON)
    return 0;

  org2.Cross(org,d2); 
  e2 = Dir^org2; 
  if(e2<=0) return 0;

  org1.Cross(d1,org);
  e1 = Dir^org1;
  if(e1<=0 || e1+e2>angle) return 0;

  b = -org^n; 
  if(b<=0)return 0;

  float invAngle = 1.0f / angle;
  dist = b * invAngle;
  u2 = e2 * invAngle;
  u1 = e1 * invAngle;
  u0 = 1.0f - u1 - u2;
  return 1;
}

sBool sRay::HitPlane(sVector31 &intersect,sVector4Arg plane) const
{
  float angle;
  float dist;

  dist = plane^Start;             // distance above plane
  if(dist<0) return 0;            // already below plane.
  angle = -(plane^Dir);           // direction
  if(angle<sEPSILON) return 0;    // ray away from plane
  
  intersect = Start + Dir*(dist/angle);
  return 1;
}

sBool sRay::HitPlaneDoubleSided(sVector31 &intersect,sVector4Arg plane) const
{
  float angle;
  float dist;

  dist = plane^Start;             // distance above plane
  angle = -(plane^Dir);           // direction
  if(dist>0)
  {
    if(angle<sEPSILON) return 0;    // ray away from plane
  }
  else
  {
    if(angle>sEPSILON) return 0;    // ray away from plane
  }
  
  intersect = Start + Dir*(dist/angle);
  return 1;
}


sBool sRay::HitSphere(float *dist, sVector31Arg center, float radius) const
{
  float r2=radius*radius;
  sVector30 dir2=center-Start;
  if ((dir2^dir2) <= r2) // ray start in sphere?
  {
    if (dist) *dist=0;
    return 0;
  }

  float l=Dir^dir2; 
  if (l<0) return 0; // point is behind ray

  sVector30 d=center-(Start+l*Dir);
  float d2=d^d;
  if (d2<=r2)
  {
    if (dist)
      *dist=l-sFSqrt(r2-d2);
    return 1;
  }
  return 0;
}


sBool sIntersectRayAABB(float &min, float &max, const sVector31 &ro, const sVector30 &ird, const sVector31 &bbmin, const sVector31 &bbmax)
{
  float near = (bbmin.x-ro.x)*ird.x;
  float far = (bbmax.x-ro.x)*ird.x;
  float temp = near;
  near = (near>far) ? far : near;
  far = (temp>far) ? temp : far;
  min = (near>min) ? near : min;
  max = (far<max) ? far : max;
  if(min>max) return sFALSE;
  //result = (min>max) ? sFALSE : result;

  near = (bbmin.y-ro.y)*ird.y;
  far = (bbmax.y-ro.y)*ird.y;
  temp = near;
  near = (near>far) ? far : near;
  far = (temp>far) ? temp : far;
  min = (near>min) ? near : min;
  max = (far<max) ? far : max;
  if(min>max) return sFALSE;
  //result = (min>max) ? sFALSE : result;

  near = (bbmin.z-ro.z)*ird.z;
  far = (bbmax.z-ro.z)*ird.z;
  temp = near;
  near = (near>far) ? far : near;
  far = (temp>far) ? temp : far;
  min = (near>min) ? near : min;
  max = (far<max) ? far : max;
  if(min>max) return sFALSE;
  //result = (min>max) ? sFALSE : result;

  return sTRUE;
}

sBool sRay::HitAABB(float &min, float &max, const sVector31& bbmin,const sVector31& bbmax)const
{
//  sBool result = sTRUE;

  sVector30 ird;
  ird.x = 1.0f / Dir.x;
  ird.y = 1.0f / Dir.y;
  ird.z = 1.0f / Dir.z;

  return sIntersectRayAABB(min,max,Start,ird,bbmin,bbmax);
  
  //for(int i=0;i<3;i++)
  //{
  //  float idir = 1.0f / Dir[i];
  //  float near = (bbmin[i]-Start[i])*idir;
  //  float far = (bbmax[i]-Start[i])*idir;
  //  if(near>far) sSwap(near,far);
  //  min = near > min ? near : min;
  //  max = far < max ? far : max;
  //  if(min>max) return sFALSE;
  //}
  //return sTRUE;
}

int sRay::IntersectPlane(float &t, sVector4Arg plane)const
{
  float nd = plane.x*Dir.x+plane.y*Dir.y+plane.z*Dir.z;
  float dist=plane.w-plane.x*Start.x-plane.y*Start.y-plane.z*Start.z;;
  if(-sEPSILON<nd && nd<sEPSILON)
  {
    t = 0.0f;
    if(-sEPSILON<dist && dist<sEPSILON)
      return 2;
    return 0;
  }
  t=dist/nd;
  return 1;
}

sBool sRay::HitBilinearPatch(float &dist,const sVector31 &p00,const sVector31 &p01,const sVector31 &p10,const sVector31 &p11,float *uOut,float *vOut) const
{
  sVERIFY((uOut && vOut) || (!uOut && !vOut))

  // Algorithm by Ramsey, Potter, Hansen, "Ray Bilinear Patch Intersections",
  // Journal of Graphics Tools Vol. 9 No. 3 (2004)
  static const float sISECT_EPSILON = 1e-6f;
  sVector30 d = p00 - Start;
  sVector30 c = p10 - p00;
  sVector30 b = p01 - p00;
  sVector30 a = (p11 - p10) - b;
  sVector30 q = Dir;

  // permute all vectors so that q.z has largest absolute value
  // this is *not* in the paper, but it's definitely necessary.
  if(sFAbs(q.x) >= sFAbs(q.y) && sFAbs(q.x) >= sFAbs(q.z))
  {
    sSwap(a.x,a.z);
    sSwap(b.x,b.z);
    sSwap(c.x,c.z);
    sSwap(d.x,d.z);
    sSwap(q.x,q.z);
  }
  else if(sFAbs(q.y) >= sFAbs(q.z))
  {
    sSwap(a.y,a.z);
    sSwap(b.y,b.z);
    sSwap(c.y,c.z);
    sSwap(d.y,d.z);
    sSwap(q.y,q.z);
  }

  float A1 = a.x*q.z - a.z*q.x, A2 = a.y*q.z - a.z*q.y;
  float B1 = b.x*q.z - b.z*q.x, B2 = b.y*q.z - b.z*q.y;
  float C1 = c.x*q.z - c.z*q.x, C2 = c.y*q.z - c.z*q.y;
  float D1 = d.x*q.z - d.z*q.x, D2 = d.y*q.z - d.z*q.y;
  
  float v[2];
  int nSolutions = sSolveQuadratic(v, A2*C1 - A1*C2, A2*D1 - A1*D2 + B2*C1 - B1*C2, B2*D1 - B1*D2);
  float currentMinT = 1e+20f;
  sBool gotOne = sFALSE;

  for(int i=0;i<nSolutions;i++)
  {
    if(v[i] < -sISECT_EPSILON || v[i] > 1.0f + sISECT_EPSILON)
      continue;

    // compute u
    float da,db,u;
    da = v[i] * A2 + B2;
    db = v[i] * (A2 - A1) + (B2 - B1);
    if(sFAbs(db) >= sFAbs(da))
      u = (v[i] * (C1 - C2) + D1 - D2) / db;
    else
      u = (-v[i] * C2 - D2) / da;

    if(u < -sISECT_EPSILON || u > 1.0f + sISECT_EPSILON)
      continue;

    // compute t
    float t = (u*v[i]*a.z + u*b.z + v[i]*c.z + d.z) / q.z;
    if(t >= 0.0f && t < currentMinT)
    {
      if(uOut) *uOut = u, *vOut = v[i];
      currentMinT = dist = t;
      gotOne = sTRUE;
    }
  }

  return gotOne;
}

/*
static float sDistRaySegmentSquared (const sRay &ray, const sVector31 &seg1, const sVector31 &seg2, float *rayparm=0)
{
  sVector30 segd = seg2-seg1;
  float extent = segd.Length();
  segd*=1/extent;

  sVector30 kDiff = ray.Start - seg1;
  float fA01 = -ray.Dir ^  segd;
  float fB0 = kDiff^ray.Dir;
  float fB1 = -kDiff^segd;
  float fC = kDiff.LengthSq();
  float fDet = sFAbs(1.0f - fA01*fA01);
  float fS0, fS1, fSqrDist, fExtDet;

  if (fDet >= sEPSILON)
  {
    // The ray and segment are not parallel.
    fS0 = fA01*fB1-fB0;
    fS1 = fA01*fB0-fB1;
    fExtDet = extent*fDet;

    if (fS0 >= 0)
    {
      if (fS1 >= -fExtDet)
      {
        if (fS1 <= fExtDet)  // region 0
        {
          // minimum at interior points of ray and segment
          float fInvDet = (1.0f)/fDet;
          fS0 *= fInvDet;
          fS1 *= fInvDet;
          fSqrDist = fS0*(fS0+fA01*fS1+(2.0f)*fB0) + fS1*(fA01*fS0+fS1+(2.0f)*fB1)+fC;
        }
        else  // region 1
        {
          fS1 = extent;
          fS0 = -(fA01*fS1+fB0);
          if (fS0 > 0)
            fSqrDist = -fS0*fS0+fS1*(fS1+(2.0f)*fB1)+fC;
          else
          {
            fS0 = 0;
            fSqrDist = fS1*(fS1+(2.0f)*fB1)+fC;
          }
        }
      }
      else  // region 5
      {
        fS1 = -extent;
        fS0 = -(fA01*fS1+fB0);
        if (fS0 > 0)
          fSqrDist = -fS0*fS0+fS1*(fS1+(2.0f)*fB1)+fC;
        else
        {
          fS0 = 0;
          fSqrDist = fS1*(fS1+(2.0f)*fB1)+fC;
        }
      }
    }
    else
    {
      if (fS1 <= -fExtDet)  // region 4
      {
        fS0 = -(-fA01*extent+fB0);
        if (fS0 > 0)
        {
          fS1 = -extent;
          fSqrDist = -fS0*fS0+fS1*(fS1+(2.0f)*fB1)+fC;
        }
        else
        {
          fS0 = 0;
          fS1 = sClamp(-fB1,-extent,extent);
          fSqrDist = fS1*(fS1+(2.0f)*fB1)+fC;
        }
      }
      else if (fS1 <= fExtDet)  // region 3
      {
        fS0 = 0;
        fS1 = sClamp(-fB1,-extent,extent);
        fSqrDist = fS1*(fS1+(2.0f)*fB1)+fC;
      }
      else  // region 2
      {
        fS0 = -(fA01*extent+fB0);
        if (fS0 > 0)
        {
          fS1 = extent;
          fSqrDist = -fS0*fS0+fS1*(fS1+(2.0f)*fB1)+fC;
        }
        else
        {
          fS0 = 0;
          fS1 = sClamp(-fB1,-extent,extent);
          fSqrDist = fS1*(fS1+(2.0f)*fB1)+fC;
        }
      }
    }
  }
  else
  {
    // ray and segment are parallel
    if (fA01 > 0)
      fS1 = -extent;
    else
      fS1 = extent;

    fS0 = -(fA01*fS1+fB0);
    if (fS0 > 0)
      fSqrDist = -fS0*fS0+fS1*(fS1+(2.0f)*fB1)+fC;
    else
      fSqrDist = fS1*(fS1+(2.0f)*fB1)+fC;
  }

  if (rayparm) *rayparm=fS0;
  return sFAbs(fSqrDist);
}


sBool sRay::HitCappedCylinder(const sVector31 &cylstart, const sVector31 &cylend, float radius, float *dist) const
{
  float d2=sDistRaySegmentSquared(*this,cylstart,cylend,dist);
  return (d2<=(radius*radius));
}
*/

/****************************************************************************/
/****************************************************************************/

void sAABBoxC::Clear()
{
  Center.Init(0,0,0);
  Radius.Init(0,0,0);
}

void sAABBoxC::Add(const sAABBoxC &box)
{
  if(Radius.x==0 && Radius.y==0 && Radius.z==0)
  {
    Radius = box.Radius;
    Center = box.Center;
  }
  else
  {
    sAABBox s;
    s.Min.x = sMin(Center.x-Radius.x,box.Center.x-box.Radius.x);
    s.Min.y = sMin(Center.y-Radius.y,box.Center.y-box.Radius.y);
    s.Min.z = sMin(Center.z-Radius.z,box.Center.z-box.Radius.z);
    s.Max.x = sMax(Center.x+Radius.x,box.Center.x+box.Radius.x);
    s.Max.y = sMax(Center.y+Radius.y,box.Center.y+box.Radius.y);
    s.Max.z = sMax(Center.z+Radius.z,box.Center.z+box.Radius.z);
    Radius = 0.5f*(s.Max-s.Min);
    Center = s.Max-Radius;
  }
}

/****************************************************************************/

sOBBox::sOBBox()
{
}

void sOBBox::Init(const sAABBox &box)
{
  BoxToWorld.Init(1,0,0,0); // identity
  Center = box.Center();
  HalfExtents.x = 0.5f * (box.Max.x - box.Min.x);
  HalfExtents.y = 0.5f * (box.Max.y - box.Min.y);
  HalfExtents.z = 0.5f * (box.Max.z - box.Min.z);
}

void sOBBox::Init(const sAABBoxC &box)
{
  BoxToWorld.Init(1,0,0,0); // identity
  Center = box.Center;
  HalfExtents = box.Radius;
}

/****************************************************************************/

void sFrustum::Init(const sMatrix44 &mat,float xMin,float xMax,float yMin,float yMax,float zMin,float zMax)
{
  const sVector4 colX(mat.i.x,mat.j.x,mat.k.x,mat.l.x);
  const sVector4 colY(mat.i.y,mat.j.y,mat.k.y,mat.l.y);
  const sVector4 colZ(mat.i.z,mat.j.z,mat.k.z,mat.l.z);
  const sVector4 colW(mat.i.w,mat.j.w,mat.k.w,mat.l.w);

  Planes[0] = colX - xMin*colW;     // sFP_Left
  Planes[1] = xMax*colW - colX;     // sFP_Right
  Planes[2] = colY - yMin*colW;     // sFP_Bottom
  Planes[3] = yMax*colW - colY;     // sFP_Top
  Planes[4] = colZ - zMin*colW;     // sFP_Near
  Planes[5] = zMax*colW - colZ;     // sFP_Far

  for(int i=0;i<6;i++)
  {
    AbsPlanes[i].x = sAbs(Planes[i].x);
    AbsPlanes[i].y = sAbs(Planes[i].y);
    AbsPlanes[i].z = sAbs(Planes[i].z);
    AbsPlanes[i].w =      Planes[i].w ;
  }
}

int sFrustum::IsInside(const sAABBoxC &b) const
{
  int result = sTEST_IN;
  for(int i=0;i<6;i++)
  {
    float m = b.Center ^ Planes[i];
    float n = b.Radius ^ AbsPlanes[i];
    if(m+n<0) return sTEST_OUT;
    if(m-n<0) result = sTEST_CLIP;
  }
  return result;
}

int sFrustum::IsInside(const sOBBox &b) const
{
  sMatrix34 mat(sDontInitialize);
  mat.Init(b.BoxToWorld);

  int result = sTEST_IN;
  for(int i=0;i<6;i++)
  {
    float m = b.Center ^ Planes[i];
    float n = b.HalfExtents.x * sFAbs(mat.i ^ Planes[i])
           + b.HalfExtents.y * sFAbs(mat.j ^ Planes[i])
           + b.HalfExtents.z * sFAbs(mat.k ^ Planes[i]);
    if(m+n<0) return sTEST_OUT;
    if(m-n<0) result = sTEST_CLIP;
  }
  return result;
}

sBool sFrustum::IsOutside(const sOBBox &b) const
{
  sMatrix34 mat(sDontInitialize);
  mat.Init(b.BoxToWorld);
  for(int i=0;i<6;i++)
  {
    float m = b.Center ^ Planes[i];
    float n = b.HalfExtents.x * sFAbs(mat.i ^ Planes[i])
           + b.HalfExtents.y * sFAbs(mat.j ^ Planes[i])
           + b.HalfExtents.z * sFAbs(mat.k ^ Planes[i]);
    if(m+n<0) return sTRUE;
  }

  return sFALSE;
}

void sFrustum::Transform(const sFrustum &src,const sMatrix34 &matx)
{
  sMatrix44 mat(matx);
  mat.Trans4();
  for(int i=0;i<6;i++)
  {
    Planes[i] = src.Planes[i]*mat;
    AbsPlanes[i].x = sAbs(Planes[i].x);
    AbsPlanes[i].y = sAbs(Planes[i].y);
    AbsPlanes[i].z = sAbs(Planes[i].z);
    AbsPlanes[i].w =      Planes[i].w ;
  }
}

void sFrustum::Transform(const sFrustum &src,const sMatrix34CM &matcm)
{
  sMatrix44 mat(matcm);
  mat.Trans4();
  for(int i=0;i<6;i++)
  {
    Planes[i] = src.Planes[i]*mat;
    AbsPlanes[i].x = sAbs(Planes[i].x);
    AbsPlanes[i].y = sAbs(Planes[i].y);
    AbsPlanes[i].z = sAbs(Planes[i].z);
    AbsPlanes[i].w =      Planes[i].w ;
  }
}

/****************************************************************************/
/****************************************************************************/

float sDistSegmentToPointSq(sVector2Arg a,sVector2Arg b,sVector2Arg p)
{
  sVector2 ab,ap,bp;

  ab = b-a;
  ap = p-a;
  bp = p-b;

  float e = ap^ab;
  if(e <= 0.0f) // outside of segment on side of a
    return ap^ap;
  else
  {
    float f = ab^ab;
    if(e >= f) // outside of segment on side of b
      return bp^bp;
    else
      return (ap^ap) - e*e/f;
  }
}

float sDistSegmentToPointSq(const sVector31 &a,const sVector31 &b,const sVector31 &p)
{
  sVector30 ab,ap,bp;

  ab = b-a;
  ap = p-a;
  bp = p-b;

  float e = ap^ab;
  if(e <= 0.0f) // outside of segment on side of a
    return ap^ap;
  else
  {
    float f = ab^ab;
    if(e >= f) // outside of segment on side of b
      return bp^bp;
    else
      return (ap^ap) - e*e/f;
  }
}

float sDistSegmentToPointSq(sVector2Arg a,sVector2Arg b,sVector2Arg p,float &t)
{
  sVector2 ab,ap,bp;

  ab = b-a;
  ap = p-a;
  bp = p-b;

  float e = ap^ab;
  if(e <= 0.0f) // outside of segment on side of a
  {
    t = 0.0f;
    return ap^ap;
  }
  else
  {
    float f = ab^ab; // >=0
    if(e >= f) // outside of segment on side of b
    {
      t = 1.0f;
      return bp^bp;
    }
    else
    {
      t = e/f;
      return (ap^ap) - e*t;
    }
  }
}

float sDistSegmentToPointSq(const sVector31 &a,const sVector31 &b,const sVector31 &p,float &t)
{
  sVector30 ab,ap,bp;

  ab = b-a;
  ap = p-a;
  bp = p-b;

  float e = ap^ab;
  if(e <= 0.0f) // outside of segment on side of a
  {
    t = 0.0f;
    return ap^ap;
  }
  else
  {
    float f = ab^ab; // >=0
    if(e >= f) // outside of segment on side of b
    {
      t = 1.0f;
      return bp^bp;
    }
    else
    {
      t = e/f;
      return (ap^ap) - e*t;
    }
  }
}

/****************************************************************************/
/****************************************************************************/

sSRT::sSRT()
{
  Scale.Init(1,1,1);
  Rotate.Init(0,0,0);
  Translate.Init(0,0,0);
}

void sSRT::Init(float *s)
{
  Scale    .Init(s[0],s[1],s[2]);
  Rotate   .Init(s[3],s[4],s[5]);
  Translate.Init(s[6],s[7],s[8]);
}

void sSRT::MakeMatrix(sMatrix34 &mat) const
{
  mat.EulerXYZ(Rotate.x*sPI2F,Rotate.y*sPI2F,Rotate.z*sPI2F);
  mat.Scale(Scale.x,Scale.y,Scale.z);
  mat.l = Translate;
}

void sSRT::MakeMatrixInv(sMatrix34 &mat) const
{
  mat.Init();
  mat.EulerXYZ(Rotate.x*sPI2F,Rotate.y*sPI2F,Rotate.z*sPI2F);  
  mat.Scale(1/Scale.x,1/Scale.y,1/Scale.z);
  mat.Trans3();
  mat.l = -Translate*mat;
}

void sSRT::Invert()
{
  sMatrix34 mat;
  MakeMatrixInv(mat);

  Translate = mat.l;
  mat.Trans3();
  Scale.x = mat.i.Length(); mat.i.Unit();
  Scale.y = mat.j.Length(); mat.j.Unit();
  Scale.z = mat.k.Length(); mat.k.Unit();
  mat.Trans3();
  mat.FindEulerXYZ(Rotate.x,Rotate.y,Rotate.z);
  Rotate.x /= sPI2F;
  Rotate.y /= sPI2F;
  Rotate.z /= sPI2F;
}

void sSRT::ToString(const sStringDesc &outStr) const
{
  uint32_t sx,sy,sz;
  uint32_t rx,ry,rz;
  uint32_t tx,ty,tz;

  sx = *((const uint32_t *) &Scale.x);
  sy = *((const uint32_t *) &Scale.y);
  sz = *((const uint32_t *) &Scale.z);
  rx = *((const uint32_t *) &Rotate.x);
  ry = *((const uint32_t *) &Rotate.y);
  rz = *((const uint32_t *) &Rotate.z);
  tx = *((const uint32_t *) &Translate.x);
  ty = *((const uint32_t *) &Translate.y);
  tz = *((const uint32_t *) &Translate.z);

  sSPrintF(outStr,L"SRT=[%08x,%08x,%08x,%08x,%08x,%08x,%08x,%08x,%08x]",
    sx,sy,sz,rx,ry,rz,tx,ty,tz);
}

sBool sSRT::FromString(const sChar *str)
{
  uint32_t fields[9];
  const sChar *p = str;

  if(sCmpStringLen(p,L"SRT=[",5) != 0)
    return sFALSE;
  
  p += 5;
  for(int i=0;i<9;i++)
  {
    if(!sScanHex(p,(int&) fields[i],8))
      return sFALSE;

    if(*p++ != ((i == 8) ? ']' : ','))
      return sFALSE;
  }

  Scale.x = *((const float *) &fields[0]);
  Scale.y = *((const float *) &fields[1]);
  Scale.z = *((const float *) &fields[2]);
  Rotate.x = *((const float *) &fields[3]);
  Rotate.y = *((const float *) &fields[4]);
  Rotate.z = *((const float *) &fields[5]);
  Translate.x = *((const float *) &fields[6]);
  Translate.y = *((const float *) &fields[7]);
  Translate.z = *((const float *) &fields[8]);

  return sTRUE;
}

/****************************************************************************/
/***                                                                      ***/
/***   Spline Basics                                                      ***/
/***                                                                      ***/
/****************************************************************************/

void sHermite(const float x1, float &c0, float &c1, float &c2, float &c3)
{
  float x2=x1*x1;
  float x3=x2*x1;

  c0 = -0.5f*x3 +      x2 - 0.5f*x1       ;
  c1 =  1.5f*x3 - 2.5f*x2           + 1.0f;
  c2 = -1.5f*x3 + 2.0f*x2 + 0.5f*x1       ;
  c3 =  0.5f*x3 - 0.5f*x2                 ;
}


void sHermiteD(const float x1, float &c0, float &c1, float &c2, float &c3)
{
  float x2=x1*x1;

  c0 = -1.5f*x2 + 2.0f*x1 - 0.5f;
  c1 =  4.5f*x2 - 5.0f*x1       ;
  c2 = -4.5f*x2 + 4.0f*x1 + 0.5f;
  c3 =  1.5f*x2 - 1.0f*x1       ;
}

void sHermiteUniform(const float x1,float t0,float t1,float t2,float &c0,float &c1,float &c2,float &c3)
{
  float x2=x1*x1;
  float x3=x2*x1;

  float d0,d1,d2,d3;
  d0 = -0.5f*x3 +      x2 - 0.5f*x1       ;
  d1 =  1.5f*x3 - 2.5f*x2           + 1.0f;
  d2 = -1.5f*x3 + 2.0f*x2 + 0.5f*x1       ;
  d3 =  0.5f*x3 - 0.5f*x2                 ;

  float f3 = t1 / t2;
  float f0 = t1 / t0;
  
  c0 = d0*f0;
  c1 = d1+d0-d0*f0;
  c2 = d2+d3-d3*f3;
  c3 = d3*f3;
}

void sHermiteUniformD(const float x1,float t0,float t1,float t2,float &c0,float &c1,float &c2,float &c3)
{
  float x2=x1*x1;

  float d0,d1,d2,d3;
  d0 = -1.5f*x2 + 2.0f*x1 - 0.5f;
  d1 =  4.5f*x2 - 5.0f*x1       ;
  d2 = -4.5f*x2 + 4.0f*x1 + 0.5f;
  d3 =  1.5f*x2 - 1.0f*x1       ;

  float f3 = t1 / t2;
  float f0 = t1 / t0;
  
  c0 = d0*f0;
  c1 = d1+d0-d0*f0;
  c2 = d2+d3-d3*f3;
  c3 = d3*f3;
}

void sHermiteWeighted(const float x1, float l0, float l1, float l2, float &c0, float &c1, float &c2, float &c3)
{
  float x2 = x1 * x1;
  float x3 = x2 * x1;
  float il01 = 1.0f / (l0 + l1);
  float il12 = 1.0f / (l1 + l2);
  float d0 = x3 - 2.0f*x2 + x1;
  float d1 = x3 - x2;

  c0 = -l0 * il01 * d0;
  c1 = 2.0f*x3 - 3.0f*x2 + 1.0f + (l0-l1) * il01 * d0 - l1 * il12 * d1;
  c2 = 3.0f*x2 - 2.0f*x3 + l1 * il01 * d0 + (l1-l2) * il12 * d1;
  c3 = l2 * il12 * d1;
}

void sUniformBSpline(float x1, float &c0, float &c1, float &c2, float &c3)
{
  float x2=x1*x1;
  float x3=x2*x1;
  static const float i6 = 1.0f / 6.0f;

  c0 =   -i6*x3 + 0.5f*x2 - 0.5f*x1 +   i6;
  c1 =  0.5f*x3 -      x2           + 4*i6;
  c2 = -0.5f*x3 + 0.5f*x2 + 0.5f*x1 +   i6;
  c3 =    i6*x3                           ;
}

void sUniformBSplineD(float x1, float &c0, float &c1, float &c2, float &c3)
{
  const float x2=x1*x1;

  c0 = -0.5f*x2 +      x1 - 0.5f;
  c1 =  1.5f*x2 - 2.0f*x1       ;
  c2 = -1.5f*x2 +      x1 + 0.5f;
  c3 =  0.5f*x2                 ;
}

void sCubicBezier(float t, float &c0, float &c1, float &c2, float &c3)
{
  float u = 1.0f - t;

  c0 = 1.0f * u*u*u;
  c1 = 3.0f * u*u*t;
  c2 = 3.0f * u*t*t;
  c3 = 1.0f * t*t*t;
}

/****************************************************************************/
/***                                                                      ***/
/***   Smooth Function                                                    ***/
/***                                                                      ***/
/****************************************************************************/

float sSmooth(float x,float s)
{
  if(x<=0) return 0;
  else if(x>=1) return 1;
  else return (s*0.5f)*(x*x) - (s-4)*(x*x)*x + (s*0.5f-3)*(x*x)*(x*x);
}

/****************************************************************************/
/***                                                                      ***/
/***   Damping timeslice syncronisation                                   ***/
/***                                                                      ***/
/****************************************************************************/

void sMakeDamp(int timeslice,float damp,float &d_,float &f_)
{
  float f,d;

  f = 1;
  d = damp;
  for(int i=1;i<timeslice;i++)
  {
    f += d;
    d *= damp;
  }
  d_ = d;
  f_ = f;
}

/****************************************************************************/
/***                                                                      ***/
/***   Filter (2Pole FIR)                                                 ***/
/***                                                                      ***/
/****************************************************************************/

void sFilter2Pole::Init()
{
  sSetMem(this,0,sizeof(*this));
}

void sFilter2Pole::Scale(float s)
{
  a0 *= s;
  a1 *= s;
  a2 *= s;
}

void sFilter2Pole::Band(float f,float bw)
{
  double c = sFCos(f*sPI2F);
  double r = 1-3*bw;
  double k = (1-2*r*c+r*r) / (2-2*c);

  a0 = 1-k;
  a1 = 2*(k-r)*c;
  a2 = r*r-k;
  b1 = 2*r*c;
  b2 = -r*r;
}



void sFilter2Pole::Low(float f,float rr)
{
  // tweaked butterworth

  double c = 1/sFTan(f*sPIF);
  double r = (1-rr)*sSQRT2F;
  
  a0 = 1/(1 + r*c + c*c);
  a1 = 2*a0;
  a2 = a0;
  b1 = -2*(1 - c*c) * a0;
  b2 = -(1 - r*c + c*c) * a0;
}

void sFilter2Pole::High(float f,float rr)
{
  // tweaked butterworth

  double c = sFTan(f*sPIF);
  double r = (1-rr)*sSQRT2F;

  a0 = 1/(1 + r*c +c*c);
  a1 = -2*a0;
  a2 = a0;
  b1 = -2*(c*c - 1)*a0;
  b2 = -(1 - r*c + c*c)*a0;
}

float sFilter2Pole::Filter(sFilter2PoleTemp &t,float sample)
{
  double x0,y0;

  x0 = sample;
  y0 = x0*a0 + t.x1*a1 + t.x2*a2 + t.y1*b1 + t.y2*b2;
 
  t.x2 = t.x1;
  t.x1 =   x0;
  t.y2 = t.y1;
  t.y1 =   y0;
  return float(y0);
}

void sFilter2Pole::Filter(sFilter2PoleTemp &t,float *in,float *out,int count)
{
  double x0,y0;

  for(int i=0;i<count;i++)
  {
    x0 = in[i];
    y0 = x0*a0 + t.x1*a1 + t.x2*a2 + t.y1*b1 + t.y2*b2;
    out[i] = float(y0);
   
    t.x2 = t.x1;
    t.x1 =   x0;
    t.y2 = t.y1;
    t.y1 =   y0;
  }
}

void sFilter2Pole::FilterStereo(sFilter2PoleTemp *t,float *in,float *out,int count)
{
  double x0,y0;

  for(int i=0;i<count;i++)
  {
    x0 = in[i*2+0];
    y0 = x0*a0 + t[0].x1*a1 + t[0].x2*a2 + t[0].y1*b1 + t[0].y2*b2;
    out[i*2+0] = float(y0);
   
    t[0].x2 = t[0].x1;
    t[0].x1 =      x0;
    t[0].y2 = t[0].y1;
    t[0].y1 =      y0;
  }
  for(int i=0;i<count;i++)
  {
    x0 = in[i*2+1];
    y0 = x0*a0 + t[1].x1*a1 + t[1].x2*a2 + t[1].y1*b1 + t[1].y2*b2;
    out[i*2+1] = float(y0);
   
    t[1].x2 = t[1].x1;
    t[1].x1 =      x0;
    t[1].y2 = t[1].y1;
    t[1].y1 =      y0;
  }
}

/****************************************************************************/
/***                                                                      ***/
/***   Perlin Noise                                                       ***/
/***                                                                      ***/
/****************************************************************************/

float sPerlinRandom2D[256][2];
float sPerlinRandom3D[256][3];
int sPerlinPermute[512];

static void sInitPerlin()
{
  int i,j;
  sRandom rnd;
  int order[256];

  rnd.Seed(0);
  // permutation
	for(i=0;i<256;i++)
	{
		order[i]=rnd.Int16();
		sPerlinPermute[i]=i;
	}

	for(i=0;i<255;i++)
	{
    for(j=i+1;j<256;j++)
    {
		  if(order[i]>order[j])
		  {
			  sSwap(order[i],order[j]);
			  sSwap(sPerlinPermute[i],sPerlinPermute[j]);
		  }
    }
	}
  for(int k=0;k<256;k++)
    sPerlinPermute[k+256] = sPerlinPermute[k];

  // random 2d
  for(i=0;i<256;)
  {
    float x,y;
    x = rnd.Float(2.0f)-1.0f;
    y = rnd.Float(2.0f)-1.0f;
    if(x*x+y*y<1.0f)
    {
      sPerlinRandom2D[i][0] = x;
      sPerlinRandom2D[i][1] = y;
      i++;
    }
  }

  // random 3d
  for(i=0;i<256;)
  {
    float x,y,z;
    x = rnd.Float(2.0f)-1.0f;
    y = rnd.Float(2.0f)-1.0f;
    z = rnd.Float(2.0f)-1.0f;
    if(x*x+y*y+z*z<1.0f)
    {
      sPerlinRandom3D[i][0] = x;
      sPerlinRandom3D[i][1] = y;
      sPerlinRandom3D[i][2] = z;
      i++;
    }
  }

}

float sPerlin2D(int x,int y,int mask,int seed)
{
  int vx0,vy0,vx1,vy1;
  int v00,v01,v10,v11;
  float f00,f01,f10,f11;
  float f0,f1,f;
  float tx,ty;

  mask &= 255;
  vx0 = (x>>16) & mask; vx1 = (vx0+1) & mask; tx=(x&0xffff)/65536.0f;
  vy0 = (y>>16) & mask; vy1 = (vy0+1) & mask; ty=(y&0xffff)/65536.0f;
  v00 = sPerlinPermute[((vx0))+sPerlinPermute[((vy0))^seed]];
  v01 = sPerlinPermute[((vx1))+sPerlinPermute[((vy0))^seed]];
  v10 = sPerlinPermute[((vx0))+sPerlinPermute[((vy1))^seed]];
  v11 = sPerlinPermute[((vx1))+sPerlinPermute[((vy1))^seed]];
  f00 = sPerlinRandom2D[v00][0]*(tx-0)+sPerlinRandom2D[v00][1]*(ty-0);
  f01 = sPerlinRandom2D[v01][0]*(tx-1)+sPerlinRandom2D[v01][1]*(ty-0);
  f10 = sPerlinRandom2D[v10][0]*(tx-0)+sPerlinRandom2D[v10][1]*(ty-1);
  f11 = sPerlinRandom2D[v11][0]*(tx-1)+sPerlinRandom2D[v11][1]*(ty-1);
  tx = tx*tx*tx*(10+tx*(6*tx-15));
  ty = ty*ty*ty*(10+ty*(6*ty-15));

  f0 = f00+(f01-f00)*tx;
  f1 = f10+(f11-f10)*tx;
  f = f0+(f1-f0)*ty;

  return f;
}

float sPerlin3D(int x,int y,int z,int mask,int seed)
{
  int vx0,vy0,vz0,vx1,vy1,vz1;
  int v000,v001,v010,v011;
  int v100,v101,v110,v111;
  float f000,f001,f010,f011;
  float f100,f101,f110,f111;
  float f00,f01,f10,f11;
  float f0,f1,f;
  float tx,ty,tz;

  mask &= 255;
  vx0 = (x>>16) & mask; vx1 = (vx0+1) & mask; tx=(x&0xffff)/65536.0f;
  vy0 = (y>>16) & mask; vy1 = (vy0+1) & mask; ty=(y&0xffff)/65536.0f;
  vz0 = (z>>16) & mask; vz1 = (vz0+1) & mask; tz=(z&0xffff)/65536.0f;
  v000 = sPerlinPermute[vx0+sPerlinPermute[vy0+sPerlinPermute[vz0^seed]]];
  v001 = sPerlinPermute[vx1+sPerlinPermute[vy0+sPerlinPermute[vz0^seed]]];
  v010 = sPerlinPermute[vx0+sPerlinPermute[vy1+sPerlinPermute[vz0^seed]]];
  v011 = sPerlinPermute[vx1+sPerlinPermute[vy1+sPerlinPermute[vz0^seed]]];
  v100 = sPerlinPermute[vx0+sPerlinPermute[vy0+sPerlinPermute[vz1^seed]]];
  v101 = sPerlinPermute[vx1+sPerlinPermute[vy0+sPerlinPermute[vz1^seed]]];
  v110 = sPerlinPermute[vx0+sPerlinPermute[vy1+sPerlinPermute[vz1^seed]]];
  v111 = sPerlinPermute[vx1+sPerlinPermute[vy1+sPerlinPermute[vz1^seed]]];

  f000 = sPerlinRandom3D[v000][0]*(tx-0)+sPerlinRandom3D[v000][1]*(ty-0)+sPerlinRandom3D[v000][2]*(tz-0);
  f001 = sPerlinRandom3D[v001][0]*(tx-1)+sPerlinRandom3D[v001][1]*(ty-0)+sPerlinRandom3D[v001][2]*(tz-0);
  f010 = sPerlinRandom3D[v010][0]*(tx-0)+sPerlinRandom3D[v010][1]*(ty-1)+sPerlinRandom3D[v010][2]*(tz-0);
  f011 = sPerlinRandom3D[v011][0]*(tx-1)+sPerlinRandom3D[v011][1]*(ty-1)+sPerlinRandom3D[v011][2]*(tz-0);
  f100 = sPerlinRandom3D[v100][0]*(tx-0)+sPerlinRandom3D[v100][1]*(ty-0)+sPerlinRandom3D[v100][2]*(tz-1);
  f101 = sPerlinRandom3D[v101][0]*(tx-1)+sPerlinRandom3D[v101][1]*(ty-0)+sPerlinRandom3D[v101][2]*(tz-1);
  f110 = sPerlinRandom3D[v110][0]*(tx-0)+sPerlinRandom3D[v110][1]*(ty-1)+sPerlinRandom3D[v110][2]*(tz-1);
  f111 = sPerlinRandom3D[v111][0]*(tx-1)+sPerlinRandom3D[v111][1]*(ty-1)+sPerlinRandom3D[v111][2]*(tz-1);
  tx = tx*tx*tx*(10+tx*(6*tx-15));
  ty = ty*ty*ty*(10+ty*(6*ty-15));
  tz = tz*tz*tz*(10+tz*(6*tz-15));

  f00 = f000+(f001-f000)*tx;
  f01 = f010+(f011-f010)*tx;
  f10 = f100+(f101-f100)*tx;
  f11 = f110+(f111-f110)*tx;

  f0 = f00+(f01-f00)*ty;
  f1 = f10+(f11-f10)*ty;

  f = f0+(f1-f0)*tz;

  return f;
}


void sPerlinDerive3D(int x,int y,int z,int mask,int seed,float &value,sVector30 &dir)
{
  int vx0,vy0,vz0,vx1,vy1,vz1;
  int v000,v001,v010,v011;
  int v100,v101,v110,v111;
  float tx,ty,tz;
  float px000,px001,px010,px011,px100,px101,px110,px111;
  float py000,py001,py010,py011,py100,py101,py110,py111;
  float pz000,pz001,pz010,pz011,pz100,pz101,pz110,pz111;
  float rx000,rx001,rx010,rx011,rx100,rx101,rx110,rx111;
  float ry000,ry001,ry010,ry011,ry100,ry101,ry110,ry111;
  float rz000,rz001,rz010,rz011,rz100,rz101,rz110,rz111;
  float ra0,ra1,rb0,rb1;

  mask &= 255;
  vx0 = (x>>16) & mask; vx1 = (vx0+1) & mask; tx=(x&0xffff)/65536.0f;
  vy0 = (y>>16) & mask; vy1 = (vy0+1) & mask; ty=(y&0xffff)/65536.0f;
  vz0 = (z>>16) & mask; vz1 = (vz0+1) & mask; tz=(z&0xffff)/65536.0f;
  v000 = sPerlinPermute[vx0+sPerlinPermute[vy0+sPerlinPermute[vz0^seed]]];
  v001 = sPerlinPermute[vx1+sPerlinPermute[vy0+sPerlinPermute[vz0^seed]]];
  v010 = sPerlinPermute[vx0+sPerlinPermute[vy1+sPerlinPermute[vz0^seed]]];
  v011 = sPerlinPermute[vx1+sPerlinPermute[vy1+sPerlinPermute[vz0^seed]]];
  v100 = sPerlinPermute[vx0+sPerlinPermute[vy0+sPerlinPermute[vz1^seed]]];
  v101 = sPerlinPermute[vx1+sPerlinPermute[vy0+sPerlinPermute[vz1^seed]]];
  v110 = sPerlinPermute[vx0+sPerlinPermute[vy1+sPerlinPermute[vz1^seed]]];
  v111 = sPerlinPermute[vx1+sPerlinPermute[vy1+sPerlinPermute[vz1^seed]]];
  px000=sPerlinRandom3D[v000][0];  py000=sPerlinRandom3D[v000][1];  pz000=sPerlinRandom3D[v000][2];
  px001=sPerlinRandom3D[v001][0];  py001=sPerlinRandom3D[v001][1];  pz001=sPerlinRandom3D[v001][2];
  px010=sPerlinRandom3D[v010][0];  py010=sPerlinRandom3D[v010][1];  pz010=sPerlinRandom3D[v010][2];
  px011=sPerlinRandom3D[v011][0];  py011=sPerlinRandom3D[v011][1];  pz011=sPerlinRandom3D[v011][2];
  px100=sPerlinRandom3D[v100][0];  py100=sPerlinRandom3D[v100][1];  pz100=sPerlinRandom3D[v100][2];
  px101=sPerlinRandom3D[v101][0];  py101=sPerlinRandom3D[v101][1];  pz101=sPerlinRandom3D[v101][2];
  px110=sPerlinRandom3D[v110][0];  py110=sPerlinRandom3D[v110][1];  pz110=sPerlinRandom3D[v110][2];
  px111=sPerlinRandom3D[v111][0];  py111=sPerlinRandom3D[v111][1];  pz111=sPerlinRandom3D[v111][2];
  
  float hx = 3*tx*tx - 2*tx*tx*tx;
  float hy = 3*ty*ty - 2*ty*ty*ty;
  float hz = 3*tz*tz - 2*tz*tz*tz;
  
  // X
  
  rx000 = px000*(1-hy)*(1-hz);
  rx001 = px001*(1-hy)*(1-hz);
  rx010 = px010*(  hy)*(1-hz);
  rx011 = px011*(  hy)*(1-hz);
  rx100 = px100*(1-hy)*(  hz);
  rx101 = px101*(1-hy)*(  hz);
  rx110 = px110*(  hy)*(  hz);
  rx111 = px111*(  hy)*(  hz);

  ry000 = py000*(1-hy)*(1-hz) * (ty-0);
  ry001 = py001*(1-hy)*(1-hz) * (ty-0);
  ry010 = py010*(  hy)*(1-hz) * (ty-1);
  ry011 = py011*(  hy)*(1-hz) * (ty-1);
  ry100 = py100*(1-hy)*(  hz) * (ty-0);
  ry101 = py101*(1-hy)*(  hz) * (ty-0);
  ry110 = py110*(  hy)*(  hz) * (ty-1);
  ry111 = py111*(  hy)*(  hz) * (ty-1);

  rz000 = pz000*(1-hy)*(1-hz) * (tz-0);
  rz001 = pz001*(1-hy)*(1-hz) * (tz-0);
  rz010 = pz010*(  hy)*(1-hz) * (tz-0);
  rz011 = pz011*(  hy)*(1-hz) * (tz-0);
  rz100 = pz100*(1-hy)*(  hz) * (tz-1);
  rz101 = pz101*(1-hy)*(  hz) * (tz-1);
  rz110 = pz110*(  hy)*(  hz) * (tz-1);
  rz111 = pz111*(  hy)*(  hz) * (tz-1);
  
  ra0 = rx000 + rx010 + rx100 + rx110;
  ra1 = rx001 + rx011 + rx101 + rx111;
  rb0 = ry000 + ry010 + ry100 + ry110 + rz000 + rz010 + rz100 + rz110;
  rb1 = ry001 + ry011 + ry101 + ry111 + rz001 + rz011 + rz101 + rz111;
 
  value = rb0 + tx*ra0 + tx*tx*3*(rb1-rb0-ra1) + tx*tx*tx*(5*ra1-3*ra0-2*rb1+2*rb0) - tx*tx*tx*tx*2*(ra1-ra0); 
  dir.x = ra0 + 6*tx*(rb1-rb0-ra1) + 3*tx*tx*(5*ra1-3*ra0-2*rb1+2*rb0) - 8*tx*tx*tx*(ra1-ra0); 

  // Y
  
  rx000 = px000*(1-hx)*(1-hz) * (tx-0);
  rx001 = px001*(  hx)*(1-hz) * (tx-1);
  rx010 = px010*(1-hx)*(1-hz) * (tx-0);
  rx011 = px011*(  hx)*(1-hz) * (tx-1);
  rx100 = px100*(1-hx)*(  hz) * (tx-0);
  rx101 = px101*(  hx)*(  hz) * (tx-1);
  rx110 = px110*(1-hx)*(  hz) * (tx-0);
  rx111 = px111*(  hx)*(  hz) * (tx-1);

  ry000 = py000*(1-hx)*(1-hz);
  ry001 = py001*(  hx)*(1-hz);
  ry010 = py010*(1-hx)*(1-hz);
  ry011 = py011*(  hx)*(1-hz);
  ry100 = py100*(1-hx)*(  hz);
  ry101 = py101*(  hx)*(  hz);
  ry110 = py110*(1-hx)*(  hz);
  ry111 = py111*(  hx)*(  hz);

  rz000 = pz000*(1-hx)*(1-hz) * (tz-0);
  rz001 = pz001*(  hx)*(1-hz) * (tz-0);
  rz010 = pz010*(1-hx)*(1-hz) * (tz-0);
  rz011 = pz011*(  hx)*(1-hz) * (tz-0);
  rz100 = pz100*(1-hx)*(  hz) * (tz-1);
  rz101 = pz101*(  hx)*(  hz) * (tz-1);
  rz110 = pz110*(1-hx)*(  hz) * (tz-1);
  rz111 = pz111*(  hx)*(  hz) * (tz-1);
  
  ra0 = ry000 + ry001 + ry100 + ry101;
  ra1 = ry010 + ry011 + ry110 + ry111;
  rb0 = rx000 + rx001 + rx100 + rx101 + rz000 + rz001 + rz100 + rz101;
  rb1 = rx010 + rx011 + rx110 + rx111 + rz010 + rz011 + rz110 + rz111;
 
  dir.y = ra0 + 6*ty*(rb1-rb0-ra1) + 3*ty*ty*(5*ra1-3*ra0-2*rb1+2*rb0) - 8*ty*ty*ty*(ra1-ra0); 

  // Z
  
  rx000 = px000*(1-hy)*(1-hx) * (tx-0);
  rx001 = px001*(1-hy)*(  hx) * (tx-1);
  rx010 = px010*(  hy)*(1-hx) * (tx-0);
  rx011 = px011*(  hy)*(  hx) * (tx-1);
  rx100 = px100*(1-hy)*(1-hx) * (tx-0);
  rx101 = px101*(1-hy)*(  hx) * (tx-1);
  rx110 = px110*(  hy)*(1-hx) * (tx-0);
  rx111 = px111*(  hy)*(  hx) * (tx-1);

  ry000 = py000*(1-hy)*(1-hx) * (ty-0);
  ry001 = py001*(1-hy)*(  hx) * (ty-0);
  ry010 = py010*(  hy)*(1-hx) * (ty-1);
  ry011 = py011*(  hy)*(  hx) * (ty-1);
  ry100 = py100*(1-hy)*(1-hx) * (ty-0);
  ry101 = py101*(1-hy)*(  hx) * (ty-0);
  ry110 = py110*(  hy)*(1-hx) * (ty-1);
  ry111 = py111*(  hy)*(  hx) * (ty-1);

  rz000 = pz000*(1-hy)*(1-hx);
  rz001 = pz001*(1-hy)*(  hx);
  rz010 = pz010*(  hy)*(1-hx);
  rz011 = pz011*(  hy)*(  hx);
  rz100 = pz100*(1-hy)*(1-hx);
  rz101 = pz101*(1-hy)*(  hx);
  rz110 = pz110*(  hy)*(1-hx);
  rz111 = pz111*(  hy)*(  hx);
  
  ra0 = rz000 + rz001 + rz010 + rz011;
  ra1 = rz100 + rz101 + rz110 + rz111;
  rb0 = rx000 + rx001 + rx010 + rx011 + ry000 + ry001 + ry010 + ry011;
  rb1 = rx100 + rx101 + rx110 + rx111 + ry100 + ry101 + ry110 + ry111;

  dir.z = ra0 + 6*tz*(rb1-rb0-ra1) + 3*tz*tz*(5*ra1-3*ra0-2*rb1+2*rb0) - 8*tz*tz*tz*(ra1-ra0); 
}

float sPerlin2D(float x,float y,int octaves,float falloff,int mode,int seed)
{
  int xi = int(x*0x10000);
  int yi = int(y*0x10000);
  int mask = 0xff;
  float sum,amp;

  sum = 0;
  amp = 1.0f;

  for(int i=0;i<octaves && i<8;i++)
  {
    float val = sPerlin2D(xi,yi,mask,seed);
    if(mode&1)
      val = (float)sFAbs(val)*2-1;
    if(mode&2)
      val = (float)sFSin(val*sPI2F);
    sum += val * amp;

    amp *= falloff;
    xi = xi*2;
    yi = yi*2;
  }

  return sum;
}

/****************************************************************************/
/***                                                                      ***/
/***  Intersection                                                        ***/
/***                                                                      ***/
/****************************************************************************/

sBool sGetIntersection(sVector2Arg u1, sVector2Arg u2, sVector2Arg v1, sVector2Arg v2, float *s, float *t, sVector2 *p)
{
  sVector2 du(u2.x - u1.x, u2.y - u1.y);
  sVector2 dv(v2.x - v1.x, v2.y - v1.y);
  float d = (dv.x * du.y - dv.y * du.x);
  if (!d)
    return sFALSE;
  float _t = ((v1.y - u1.y) * du.x - (v1.x - u1.x) * du.y) / d;
  if ((_t < 0)||(_t > 1))
    return sFALSE;
  float _s;
  if (sAbs(du.y) > 0.00001f * sMax(1.0f, sMax(sAbs(u1.y), sAbs(u2.y))))
    _s = (v1.y - u1.y + _t * dv.y) / du.y;
  else
    _s = (v1.x - u1.x + _t * dv.x) / du.x;
  if ((_s < 0)||(_s > 1))
    return sFALSE;
  if (t)
    *t = _t;
  if (s)
    *s = _s;
  if (p)
    p->Init(v1.x + dv.x * _t, v1.y + dv.y * _t);
  return sTRUE;
}

sBool sGetIntersection(sVector2Arg u1, sVector2Arg u2, const sFRect &rect, float *s, sVector2 *p)
{  
  sVector2 _p;
  if ((sGetIntersection(u1, u2, sVector2(rect.x0, rect.y0), sVector2(rect.x1, rect.y1), s, sNULL, &_p) && rect.Hit(_p.x, _p.y))
    || (sGetIntersection(u1, u2, sVector2(rect.x1, rect.y0), sVector2(rect.x0, rect.y1), s, sNULL, &_p) && rect.Hit(_p.x, _p.y)))
  {
    if (p) *p = _p;
    return sTRUE;
  }
  return sFALSE;
}

int sGetLineSegmentIntersection(float &dist0, float &dist1, sVector2Arg ls0, sVector2Arg ld0, sVector2Arg ls1, sVector2Arg ld1)
{
  float denom = ld1.y*ld0.x-ld1.x*ld0.y;
  float nume_a = ld1.x*(ls0.y-ls1.y)-ld1.y*(ls0.x-ls1.x);
  float nume_b = ld0.x*(ls0.y-ls1.y)-ld0.y*(ls0.x-ls1.x);

  if(sFAbs(denom)<0.00001f)
  {
    if(nume_a==0.0f && nume_b==0.0f)
    {
      // coincident lines, check if segments overlap
      float lengthsqr = ld0.x*ld0.x+ld0.y*ld0.y;
      float length0 = ld0.x*ls1.x+ld0.y*ls1.y;
      float length1 = ld0.x*(ls1.x+ld1.x)+ld0.y*(ls1.y+ld1.y);
      if(length1<length0)
        sSwap(length0,length1);
      if(length0<0.0f)
        length0 = 0.0f;
      if(length1>lengthsqr)
        length1 = lengthsqr;
      if(length1<length0)
        return 0;         // line segments not intersecting
      float invlsqr = 1.0f / lengthsqr;
      dist0 = length0*invlsqr;
      dist1 = length1*invlsqr;
      return 2;
    }
    return 0;
  }
  float invdenom = 1.0f/denom;
  dist0 = nume_a*invdenom;
  dist1 = nume_b*invdenom;

  if(dist0>=0.0f&&dist0<=1.0f&&dist1>=-1.0f&&dist1<=1.0f)
    return 1;
  return 0;
}

/****************************************************************************/
/***                                                                      ***/
/***   Bezier Splines                                                     ***/
/***                                                                      ***/
/****************************************************************************/

sVector30 sGetBezierSplineFactors3(float n)
{
  float m = (1.0f-n);
  return sVector30(
    m*m, 
    2.0f*n*m, 
    n*n);
}

sVector4 sGetBezierSplineFactors4(float n)
{
  float m = (1.0f-n);
  return sVector4(
    m*m*m,
    3.0f*n*m*m,
    3.0f*n*n*m,
    n*n*n);
}

void sGetBezierSplinePoint(float *out, float n, const float *v1, const float *v2, const float *v3, const uint32_t d)
{
  sVector30 f = sGetBezierSplineFactors3(n);
  for (uint32_t i = 0; i != d; ++i)
  {
    out[i] = v1[i] * f[0] + v2[i] * f[1] + v3[i] * f[2];
  }
}

void sGetBezierSplinePoint(float *out, float n, const float *v1, const float *v2, const float *v3, const float *v4, const uint32_t d)
{
  sVector4 f = sGetBezierSplineFactors4(n);
  for (uint32_t i = 0; i != d; ++i)
  {
    out[i] = v1[i] * f[0] + v2[i] * f[1] + v3[i] * f[2] + v4[i] * f[3];
  }
}

/****************************************************************************/
/***                                                                      ***/
/***   RGB <-> HSL color space conversion                                 ***/
/***                                                                      ***/
/****************************************************************************/

// HSL: Hue, Saturation, Luminance
// H: position in the spectrum
// S: color saturation
// L: color lightness

void sRgbToHsl(float r, float g, float b, float &h, float &s, float &l)
{
  float maxc = sMax(sMax(r,g),b);
  float minc = sMin(sMin(r,g),b);
  l = (minc+maxc)/2.0f;
  if (minc == maxc)
  {
    h = 0.0f; s = 0.0f;
    return;
  }
  if (l <= 0.5f)
    s = (maxc - minc) / (maxc + minc);
  else
    s = (maxc - minc) / (2.0f-maxc-minc);
  float rc = (maxc-r) / (maxc-minc);
  float gc = (maxc-g) / (maxc-minc);
  float bc = (maxc-b) / (maxc-minc);
  if (r == maxc)
    h = bc-gc;
  else if (g == maxc)
    h = 2.0f+rc-bc;
  else
    h = 4.0f+gc-rc;
  h = sAbsMod(h/6.0f, 1.0f);

}

static float _sHslToRgb_GetChannel(float m1, float m2, float hue)
{
  hue = sAbsMod(hue, 1.0f);
  if (hue < (1.0f/6.0f))
    return m1 + (m2-m1)*hue*6.0f;
  if (hue < 0.5f)
    return m2;
  if (hue < (2.0f/3.0f))
    return m1 + (m2-m1)*((2.0f/3.0f)-hue)*6.0f;
  return m1;
}

void sHslToRgb(float h, float s, float l, float &r, float &g, float &b)
{
  if (s == 0.0f)
  {
    r = l;
    g = l;
    b = l;
    return;
  }
  float m1,m2;
  if (l <= 0.5f)
    m2 = l * (1.0f+s);
  else
    m2 = l+s-(l*s);
  m1 = 2.0f*l - m2;
  r = _sHslToRgb_GetChannel(m1, m2, h+(1.0f/3.0f));
  g = _sHslToRgb_GetChannel(m1, m2, h);
  b = _sHslToRgb_GetChannel(m1, m2, h-(1.0f/3.0f));
}

void sHslToRgb(const sVector4 &in, sVector4 &out)
{
  out.w = in.w;
  sHslToRgb(in.x, in.y, in.z, out.x, out.y, out.z);
}

uint32_t sHslToColor(float h, float s, float l, float a)
{
  sVector4 v(h,s,l,a);
  sHslToRgb(v,v);
  return v.GetColor();
}

/****************************************************************************/
