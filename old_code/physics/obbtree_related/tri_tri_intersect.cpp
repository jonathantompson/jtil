/* Triangle/triangle intersection test routine,
 * by Tomas Moller, 1997.
 * See article "A Fast Triangle-Triangle Intersection Test",
 * Journal of Graphics Tools, 2(2), 1997
 * updated: 2001-06-20 (added line of intersection)
 *
 * int tri_tri_intersect(float V0[3],float V1[3],float V2[3],
 *                       float U0[3],float U1[3],float U2[3])
 *
 * parameters: vertices of triangle 1: V0,V1,V2
 *             vertices of triangle 2: U0,U1,U2
 * result    : returns 1 if the triangles intersect, otherwise 0
 *
 * Here is a version withouts divisions (a little faster)
 * int NoDivTriTriIsect(float V0[3],float V1[3],float V2[3],
 *                      float U0[3],float U1[3],float U2[3]);
 * 
 * This version computes the line of intersection as well (if they are not coplanar):
 * int tri_tri_intersect_with_isectline(float V0[3],float V1[3],float V2[3], 
 *				        float U0[3],float U1[3],float U2[3],int *coplanar,
 *				        float isectpt1[3],float isectpt2[3]);
 * coplanar returns whether the tris are coplanar
 * isectpt1, isectpt2 are the endpoints of the line of intersection
 */

#include "physics\obbtree_related\triTriIntersect.h"

#include "utils_and_misc_classes\new_redefine.h" // MUST COME LAST

#define EPSILON								0.00000001 // A small value (with some margin)

int coplanar_tri_tri(float N[3],float V0[3],float V1[3],float V2[3],
					 float U0[3],float U1[3],float U2[3])
{
	float A[3];
	short i0,i1;
	/* first project onto an axis-aligned plane, that maximizes the area */
	/* of the triangles, compute indices: i0,i1. */
	A[0]=fabs(N[0]);
	A[1]=fabs(N[1]);
	A[2]=fabs(N[2]);
	if(A[0]>A[1])
	{
		if(A[0]>A[2])  
		{
			i0=1;      /* A[0] is greatest */
			i1=2;
		}
		else
		{
			i0=0;      /* A[2] is greatest */
			i1=1;
		}
	}
	else   /* A[0]<=A[1] */
	{
		if(A[2]>A[1])
		{
			i0=0;      /* A[2] is greatest */
			i1=1;                                           
		}
		else
		{
			i0=0;      /* A[1] is greatest */
			i1=2;
		}
	}               

	/* test all edges of triangle 1 against the edges of triangle 2 */
	EDGE_AGAINST_TRI_EDGES(V0,V1,U0,U1,U2);
	EDGE_AGAINST_TRI_EDGES(V1,V2,U0,U1,U2);
	EDGE_AGAINST_TRI_EDGES(V2,V0,U0,U1,U2);

	/* finally, test if tri1 is totally contained in tri2 or vice versa */
	POINT_IN_TRI(V0,U0,U1,U2);
	POINT_IN_TRI(U0,V0,V1,V2);

	return 0;
}

int tri_tri_intersect(float V0[3],float V1[3],float V2[3],
					  float U0[3],float U1[3],float U2[3])
{
	float E1[3],E2[3];
	float N1[3],N2[3],d1,d2;
	float du0,du1,du2,dv0,dv1,dv2;
	float D[3];
	float isect1[2], isect2[2];
	float du0du1,du0du2,dv0dv1,dv0dv2;
	short index;
	float vp0,vp1,vp2;
	float up0,up1,up2;
	float b,c,max;

	/* compute plane equation of triangle(V0,V1,V2) */
	SUB(E1,V1,V0);
	SUB(E2,V2,V0);
	CROSS(N1,E1,E2);
	d1=-DOT(N1,V0);
	/* plane equation 1: N1.X+d1=0 */

	/* put U0,U1,U2 into plane equation 1 to compute signed distances to the plane*/
	du0=DOT(N1,U0)+d1;
	du1=DOT(N1,U1)+d1;
	du2=DOT(N1,U2)+d1;

	/* coplanarity robustness check */
#if USE_EPSILON_TEST==TRUE
	if(fabs(du0)<EPSILON) du0=0.0;
	if(fabs(du1)<EPSILON) du1=0.0;
	if(fabs(du2)<EPSILON) du2=0.0;
#endif
	du0du1=du0*du1;
	du0du2=du0*du2;

	if(du0du1>0.0f && du0du2>0.0f) /* same sign on all of them + not equal 0 ? */
		return 0;                    /* no intersection occurs */

	/* compute plane of triangle (U0,U1,U2) */
	SUB(E1,U1,U0);
	SUB(E2,U2,U0);
	CROSS(N2,E1,E2);
	d2=-DOT(N2,U0);
	/* plane equation 2: N2.X+d2=0 */

	/* put V0,V1,V2 into plane equation 2 */
	dv0=DOT(N2,V0)+d2;
	dv1=DOT(N2,V1)+d2;
	dv2=DOT(N2,V2)+d2;

#if USE_EPSILON_TEST==TRUE
	if(fabs(dv0)<EPSILON) dv0=0.0;
	if(fabs(dv1)<EPSILON) dv1=0.0;
	if(fabs(dv2)<EPSILON) dv2=0.0;
#endif

	dv0dv1=dv0*dv1;
	dv0dv2=dv0*dv2;

	if(dv0dv1>0.0f && dv0dv2>0.0f) /* same sign on all of them + not equal 0 ? */
		return 0;                    /* no intersection occurs */

	/* compute direction of intersection line */
	CROSS(D,N1,N2);

	/* compute and index to the largest component of D */
	max=fabs(D[0]);
	index=0;
	b=fabs(D[1]);
	c=fabs(D[2]);
	if(b>max) max=b,index=1;
	if(c>max) max=c,index=2;

	/* this is the simplified projection onto L*/
	vp0=V0[index];
	vp1=V1[index];
	vp2=V2[index];

	up0=U0[index];
	up1=U1[index];
	up2=U2[index];

	/* compute interval for triangle 1 */
	COMPUTE_INTERVALS(vp0,vp1,vp2,dv0,dv1,dv2,dv0dv1,dv0dv2,isect1[0],isect1[1]);

	/* compute interval for triangle 2 */
	COMPUTE_INTERVALS(up0,up1,up2,du0,du1,du2,du0du1,du0du2,isect2[0],isect2[1]);

	SORT(isect1[0],isect1[1]);
	SORT(isect2[0],isect2[1]);

	if(isect1[1]<isect2[0] || isect2[1]<isect1[0]) return 0;
	return 1;
}

int NoDivTriTriIsect(float V0[3],float V1[3],float V2[3],
					 float U0[3],float U1[3],float U2[3])
{
	float E1[3],E2[3];
	float N1[3],N2[3],d1,d2;
	float du0,du1,du2,dv0,dv1,dv2;
	float D[3];
	float isect1[2], isect2[2];
	float du0du1,du0du2,dv0dv1,dv0dv2;
	short index;
	float vp0,vp1,vp2;
	float up0,up1,up2;
	float bb,cc,max;
	float a,b,c,x0,x1;
	float d,e,f,y0,y1;
	float xx,yy,xxyy,tmp;

	/* compute plane equation of triangle(V0,V1,V2) */
	SUB(E1,V1,V0);
	SUB(E2,V2,V0);
	CROSS(N1,E1,E2);
	d1=-DOT(N1,V0);
	/* plane equation 1: N1.X+d1=0 */

	/* put U0,U1,U2 into plane equation 1 to compute signed distances to the plane*/
	du0=DOT(N1,U0)+d1;
	du1=DOT(N1,U1)+d1;
	du2=DOT(N1,U2)+d1;

	/* coplanarity robustness check */
#if USE_EPSILON_TEST==TRUE
	if(FABS(du0)<EPSILON) du0=0.0;
	if(FABS(du1)<EPSILON) du1=0.0;
	if(FABS(du2)<EPSILON) du2=0.0;
#endif
	du0du1=du0*du1;
	du0du2=du0*du2;

	if(du0du1>0.0f && du0du2>0.0f) /* same sign on all of them + not equal 0 ? */
		return 0;                    /* no intersection occurs */

	/* compute plane of triangle (U0,U1,U2) */
	SUB(E1,U1,U0);
	SUB(E2,U2,U0);
	CROSS(N2,E1,E2);
	d2=-DOT(N2,U0);
	/* plane equation 2: N2.X+d2=0 */

	/* put V0,V1,V2 into plane equation 2 */
	dv0=DOT(N2,V0)+d2;
	dv1=DOT(N2,V1)+d2;
	dv2=DOT(N2,V2)+d2;

#if USE_EPSILON_TEST==TRUE
	if(FABS(dv0)<EPSILON) dv0=0.0;
	if(FABS(dv1)<EPSILON) dv1=0.0;
	if(FABS(dv2)<EPSILON) dv2=0.0;
#endif

	dv0dv1=dv0*dv1;
	dv0dv2=dv0*dv2;

	if(dv0dv1>0.0f && dv0dv2>0.0f) /* same sign on all of them + not equal 0 ? */
		return 0;                    /* no intersection occurs */

	/* compute direction of intersection line */
	CROSS(D,N1,N2);

	/* compute and index to the largest component of D */
	max=(float)FABS(D[0]);
	index=0;
	bb=(float)FABS(D[1]);
	cc=(float)FABS(D[2]);
	if(bb>max) max=bb,index=1;
	if(cc>max) max=cc,index=2;

	/* this is the simplified projection onto L*/
	vp0=V0[index];
	vp1=V1[index];
	vp2=V2[index];

	up0=U0[index];
	up1=U1[index];
	up2=U2[index];

	/* compute interval for triangle 1 */
	NEWCOMPUTE_INTERVALS(vp0,vp1,vp2,dv0,dv1,dv2,dv0dv1,dv0dv2,a,b,c,x0,x1);

	/* compute interval for triangle 2 */
	NEWCOMPUTE_INTERVALS(up0,up1,up2,du0,du1,du2,du0du1,du0du2,d,e,f,y0,y1);

	xx=x0*x1;
	yy=y0*y1;
	xxyy=xx*yy;

	tmp=a*xxyy;
	isect1[0]=tmp+b*x1*yy;
	isect1[1]=tmp+c*x0*yy;

	tmp=d*xxyy;
	isect2[0]=tmp+e*xx*y1;
	isect2[1]=tmp+f*xx*y0;

	SORT(isect1[0],isect1[1]);
	SORT(isect2[0],isect2[1]);

	if(isect1[1]<isect2[0] || isect2[1]<isect1[0]) return 0;
	return 1;
}

inline void isect2(float VTX0[3],float VTX1[3],float VTX2[3],float VV0,float VV1,float VV2,
	    float D0,float D1,float D2,float *isect0,float *isect1,float isectpoint0[3],float isectpoint1[3]) 
{
  float tmp=D0/(D0-D1);          
  float diff[3];
  *isect0=VV0+(VV1-VV0)*tmp;         
  SUB(diff,VTX1,VTX0);              
  MULT(diff,diff,tmp);               
  ADD(isectpoint0,diff,VTX0);        
  tmp=D0/(D0-D2);                    
  *isect1=VV0+(VV2-VV0)*tmp;          
  SUB(diff,VTX2,VTX0);                   
  MULT(diff,diff,tmp);                 
  ADD(isectpoint1,VTX0,diff);          
}

#if 0
#define ISECT2(VTX0,VTX1,VTX2,VV0,VV1,VV2,D0,D1,D2,isect0,isect1,isectpoint0,isectpoint1) \
              tmp=D0/(D0-D1);                    \
              isect0=VV0+(VV1-VV0)*tmp;          \
	      SUB(diff,VTX1,VTX0);               \
	      MULT(diff,diff,tmp);               \
              ADD(isectpoint0,diff,VTX0);        \ 
              tmp=D0/(D0-D2);                    
/*              isect1=VV0+(VV2-VV0)*tmp;          \ */
/*              SUB(diff,VTX2,VTX0);               \     */
/*              MULT(diff,diff,tmp);               \   */
/*              ADD(isectpoint1,VTX0,diff);           */
#endif

inline int compute_intervals_isectline(float VERT0[3],float VERT1[3],float VERT2[3],
				       float VV0,float VV1,float VV2,float D0,float D1,float D2,
				       float D0D1,float D0D2,float *isect0,float *isect1,
				       float isectpoint0[3],float isectpoint1[3])
{
  if(D0D1>0.0f)                                        
  {                                                    
    /* here we know that D0D2<=0.0 */                  
    /* that is D0, D1 are on the same side, D2 on the other or on the plane */
    isect2(VERT2,VERT0,VERT1,VV2,VV0,VV1,D2,D0,D1,isect0,isect1,isectpoint0,isectpoint1);
  } 
  else if(D0D2>0.0f)                                   
    {                                                   
    /* here we know that d0d1<=0.0 */             
    isect2(VERT1,VERT0,VERT2,VV1,VV0,VV2,D1,D0,D2,isect0,isect1,isectpoint0,isectpoint1);
  }                                                  
  else if(D1*D2>0.0f || D0!=0.0f)   
  {                                   
    /* here we know that d0d1<=0.0 or that D0!=0.0 */
    isect2(VERT0,VERT1,VERT2,VV0,VV1,VV2,D0,D1,D2,isect0,isect1,isectpoint0,isectpoint1);   
  }                                                  
  else if(D1!=0.0f)                                  
  {                                               
    isect2(VERT1,VERT0,VERT2,VV1,VV0,VV2,D1,D0,D2,isect0,isect1,isectpoint0,isectpoint1); 
  }                                         
  else if(D2!=0.0f)                                  
  {                                                   
    isect2(VERT2,VERT0,VERT1,VV2,VV0,VV1,D2,D0,D1,isect0,isect1,isectpoint0,isectpoint1);     
  }                                                 
  else                                               
  {                                                   
    /* triangles are coplanar */    
    return 1;
  }
  return 0;
}

#define COMPUTE_INTERVALS_ISECTLINE(VERT0,VERT1,VERT2,VV0,VV1,VV2,D0,D1,D2,D0D1,D0D2,isect0,isect1,isectpoint0,isectpoint1) \
  if(D0D1>0.0f)                                         \
  {                                                     \
    /* here we know that D0D2<=0.0 */                   \
    /* that is D0, D1 are on the same side, D2 on the other or on the plane */ \
    isect2(VERT2,VERT0,VERT1,VV2,VV0,VV1,D2,D0,D1,&isect0,&isect1,isectpoint0,isectpoint1);          \
  }                                                     
#if 0
  else if(D0D2>0.0f)                                    \
  {                                                     \
    /* here we know that d0d1<=0.0 */                   \
    isect2(VERT1,VERT0,VERT2,VV1,VV0,VV2,D1,D0,D2,&isect0,&isect1,isectpoint0,isectpoint1);          \
  }                                                     \
  else if(D1*D2>0.0f || D0!=0.0f)                       \
  {                                                     \
    /* here we know that d0d1<=0.0 or that D0!=0.0 */   \
    isect2(VERT0,VERT1,VERT2,VV0,VV1,VV2,D0,D1,D2,&isect0,&isect1,isectpoint0,isectpoint1);          \
  }                                                     \
  else if(D1!=0.0f)                                     \
  {                                                     \
    isect2(VERT1,VERT0,VERT2,VV1,VV0,VV2,D1,D0,D2,&isect0,&isect1,isectpoint0,isectpoint1);          \
  }                                                     \
  else if(D2!=0.0f)                                     \
  {                                                     \
    isect2(VERT2,VERT0,VERT1,VV2,VV0,VV1,D2,D0,D1,&isect0,&isect1,isectpoint0,isectpoint1);          \
  }                                                     \
  else                                                  \
  {                                                     \
    /* triangles are coplanar */                        \
    coplanar=1;                                         \
    return coplanar_tri_tri(N1,V0,V1,V2,U0,U1,U2);      \
  }

#endif

int tri_tri_intersect_with_isectline(float V0[3],float V1[3],float V2[3],
				     float U0[3],float U1[3],float U2[3],int *coplanar,
				     float isectpt1[3],float isectpt2[3])
{
  float E1[3],E2[3];
  float N1[3],N2[3],d1,d2;
  float du0,du1,du2,dv0,dv1,dv2;
  float D[3];
  float isect1[2], isect2[2];
  float isectpointA1[3],isectpointA2[3];
  float isectpointB1[3],isectpointB2[3];
  float du0du1,du0du2,dv0dv1,dv0dv2;
  short index;
  float vp0,vp1,vp2;
  float up0,up1,up2;
  float b,c,max;
//  float tmp,diff[3];
  int smallest1,smallest2;
  
  /* compute plane equation of triangle(V0,V1,V2) */
  SUB(E1,V1,V0);
  SUB(E2,V2,V0);
  CROSS(N1,E1,E2);
  d1=-DOT(N1,V0);
  /* plane equation 1: N1.X+d1=0 */

  /* put U0,U1,U2 into plane equation 1 to compute signed distances to the plane*/
  du0=DOT(N1,U0)+d1;
  du1=DOT(N1,U1)+d1;
  du2=DOT(N1,U2)+d1;

  /* coplanarity robustness check */
#if USE_EPSILON_TEST==TRUE
  if(fabs(du0)<EPSILON) du0=0.0;
  if(fabs(du1)<EPSILON) du1=0.0;
  if(fabs(du2)<EPSILON) du2=0.0;
#endif
  du0du1=du0*du1;
  du0du2=du0*du2;

  if(du0du1>0.0f && du0du2>0.0f) /* same sign on all of them + not equal 0 ? */
    return 0;                    /* no intersection occurs */

  /* compute plane of triangle (U0,U1,U2) */
  SUB(E1,U1,U0);
  SUB(E2,U2,U0);
  CROSS(N2,E1,E2);
  d2=-DOT(N2,U0);
  /* plane equation 2: N2.X+d2=0 */

  /* put V0,V1,V2 into plane equation 2 */
  dv0=DOT(N2,V0)+d2;
  dv1=DOT(N2,V1)+d2;
  dv2=DOT(N2,V2)+d2;

#if USE_EPSILON_TEST==TRUE
  if(fabs(dv0)<EPSILON) dv0=0.0;
  if(fabs(dv1)<EPSILON) dv1=0.0;
  if(fabs(dv2)<EPSILON) dv2=0.0;
#endif

  dv0dv1=dv0*dv1;
  dv0dv2=dv0*dv2;
        
  if(dv0dv1>0.0f && dv0dv2>0.0f) /* same sign on all of them + not equal 0 ? */
    return 0;                    /* no intersection occurs */

  /* compute direction of intersection line */
  CROSS(D,N1,N2);

  /* compute and index to the largest component of D */
  max=fabs(D[0]);
  index=0;
  b=fabs(D[1]);
  c=fabs(D[2]);
  if(b>max) max=b,index=1;
  if(c>max) max=c,index=2;

  /* this is the simplified projection onto L*/
  vp0=V0[index];
  vp1=V1[index];
  vp2=V2[index];
  
  up0=U0[index];
  up1=U1[index];
  up2=U2[index];

  /* compute interval for triangle 1 */
  *coplanar=compute_intervals_isectline(V0,V1,V2,vp0,vp1,vp2,dv0,dv1,dv2,
				       dv0dv1,dv0dv2,&isect1[0],&isect1[1],isectpointA1,isectpointA2);
  if(*coplanar) return coplanar_tri_tri(N1,V0,V1,V2,U0,U1,U2);     


  /* compute interval for triangle 2 */
  compute_intervals_isectline(U0,U1,U2,up0,up1,up2,du0,du1,du2,
			      du0du1,du0du2,&isect2[0],&isect2[1],isectpointB1,isectpointB2);

  SORT2(isect1[0],isect1[1],smallest1);
  SORT2(isect2[0],isect2[1],smallest2);

  if(isect1[1]<isect2[0] || isect2[1]<isect1[0]) return 0;

  /* at this point, we know that the triangles intersect */

  if(isect2[0]<isect1[0])
  {
    if(smallest1==0) { SET(isectpt1,isectpointA1); }
    else { SET(isectpt1,isectpointA2); }

    if(isect2[1]<isect1[1])
    {
      if(smallest2==0) { SET(isectpt2,isectpointB2); }
      else { SET(isectpt2,isectpointB1); }
    }
    else
    {
      if(smallest1==0) { SET(isectpt2,isectpointA2); }
      else { SET(isectpt2,isectpointA1); }
    }
  }
  else
  {
    if(smallest2==0) { SET(isectpt1,isectpointB1); }
    else { SET(isectpt1,isectpointB2); }

    if(isect2[1]>isect1[1])
    {
      if(smallest1==0) { SET(isectpt2,isectpointA2); }
      else { SET(isectpt2,isectpointA1); }      
    }
    else
    {
      if(smallest2==0) { SET(isectpt2,isectpointB2); }
      else { SET(isectpt2,isectpointB1); } 
    }
  }
  return 1;
}
