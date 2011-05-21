/*
 *                This source code is part of
 * 
 *                     E  R  K  A  L  E
 *                             -
 *                       DFT from Hel
 *
 * Written by Jussi Lehtola, 2010-2011
 * Copyright (c) 2010-2011, Jussi Lehtola
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */



#include <armadillo>
#include <cfloat>
#include <cstdio>
#include <sstream>
#include <stdexcept>

#include "obara-saika.h"
#include "integrals.h"
#include "mathf.h"

// For comparison against Huzinaga integrals
//#define DEBUG


arma::mat overlap_int_os(double xa, double ya, double za, double zetaa, const std::vector<shellf_t> & carta, double xb, double yb, double zb, double zetab, const std::vector<shellf_t> & cartb) {
  // Compute shell of overlap integrals

  // Angular momenta of shells
  int am_a=carta[0].l+carta[0].m+carta[0].n;
  int am_b=cartb[0].l+cartb[0].m+cartb[0].n;

  // Returned matrix
  arma::mat S(carta.size(),cartb.size());
  S.zeros();

  // Get 1d overlaps
  arma::mat ox=overlap_ints_1d(xa,xb,zetaa,zetab,am_a,am_b);
  arma::mat oy=overlap_ints_1d(ya,yb,zetaa,zetab,am_a,am_b);
  arma::mat oz=overlap_ints_1d(za,zb,zetaa,zetab,am_a,am_b);

  int la, ma, na;
  int lb, mb, nb;
  double norma, normb;

  for(size_t i=0;i<carta.size();i++) {
      la=carta[i].l;
      ma=carta[i].m;
      na=carta[i].n;
      norma=carta[i].relnorm;

    for(size_t j=0;j<cartb.size();j++) {
      lb=cartb[j].l;
      mb=cartb[j].m;
      nb=cartb[j].n;
      normb=cartb[j].relnorm;

      S(i,j)=norma*normb*ox(la,lb)*oy(ma,mb)*oz(na,nb);

    }
  }

#ifdef DEBUG
  arma::mat huz(carta.size(),cartb.size());
  for(size_t i=0;i<carta.size();i++) {
    la=carta[i].l;
    ma=carta[i].m;
    na=carta[i].n;
    norma=carta[i].relnorm;

    for(size_t j=0;j<cartb.size();j++) {
      lb=cartb[j].l;
      mb=cartb[j].m;
      nb=cartb[j].n;
      normb=cartb[j].relnorm;

      huz(i,j)=norma*normb*overlap_int(xa,ya,za,zetaa,la,ma,na,xb,yb,zb,zetab,lb,mb,nb);
    }
  }

  int diff=0;
  for(size_t i=0;i<carta.size();i++)
    for(size_t j=0;j<cartb.size();j++) 
      if(fabs(S(i,j)-huz(i,j))>10*DBL_EPSILON*fabs(huz(i,j)))
	diff++;

  if(diff==0) 
    printf("Computed shell of overlaps (%e,%e,%e)-(%e,%e,%e) with zeta=(%e,%e) and am=(%i,%i), the results match.\n",xa,ya,za,xb,yb,zb,zetaa,zetab,am_a,am_b);
  else
      for(size_t i=0;i<carta.size();i++)
	for(size_t j=0;j<cartb.size();j++) 
	  if(fabs(S(i,j)-huz(i,j))>10*DBL_EPSILON*fabs(huz(i,j))) {
	    printf("Computed overlap (%e,%e,%e)-(%e,%e,%e) with zeta=(%e,%e) and am=(%i,%i,%i)-(%i,%i,%i)\n",xa,ya,za,xb,yb,zb,zetaa,zetab,la,ma,na,lb,mb,nb);
	    printf("Huzinaga gives %e, OS gives %e.\n",huz(i,j),S(i,j));
	  }

#endif

  return S;
}
	
double overlap_int_os(double xa, double ya, double za, double zetaa, int la, int ma, int na, double xb, double yb, double zb, double zetab, int lb, int mb, int nb) {
  return overlap_int_1d(xa,xb,zetaa,zetab,la,lb)*overlap_int_1d(ya,yb,zetaa,zetab,ma,mb)*overlap_int_1d(za,zb,zetaa,zetab,na,nb);
}

double overlap_int_1d(double xa, double xb, double zetaa, double zetab, int la, int lb) {
  return overlap_ints_1d(xa,xb,zetaa,zetab,la,lb)(la,lb);
}

arma::mat overlap_ints_1d(double xa, double xb, double zetaa, double zetab, int la, int lb) {
  // In the following we assume la>lb.
  if(lb<la) // Switch arguments if necessary.
    return trans(overlap_ints_1d(xb,xa,zetab,zetaa,lb,la));

  // Compute exponents
  double p=zetaa+zetab;
  double mu=zetaa*zetab/(zetaa+zetab);

  // Compute center
  double px=(zetaa*xa+zetab*xb)/p;

  double xab=xa-xb;
  double xpb=px-xb;

  // We want to compute S_{la lb} with recurrence relations
  // S_{i+1,j} = X_{PA} S_{i,j} + 1/2p * (iS_{i-1,j}+jS_{i,j-1})
  // S_{i,j+1} = X_{PB} S_{i,j} + 1/2p * (iS_{i-1,j}+jS_{i,j-1})

  // We need some extra work space to use the recursion relations
  int lawrk=la+1;
  int lbwrk=lb+la+2;

  arma::mat S(lawrk,lbwrk);
  S.zeros();

  // Initialize S_{00}
  S(0,0)=sqrt(M_PI/p)*exp(-mu*xab*xab);

  if(la>0 || lb>0) {
    
    // Generate integrals S_{0,j}
    S(0,1)=xpb*S(0,0);
    for(int j=1;j<lbwrk-1;j++) {
      S(0,j+1)=xpb*S(0,j)+0.5/p*j*S(0,j-1);
    }

    // Use horizontal recurrence to generate S_{ij}
    // S_{i+1,j} = S_{i,j+1} - X_{AB} S_{ij}
    for(int i=0;i<la;i++) {
      for(int j=0;j<lb+la-i;j++) {
	S(i+1,j)=S(i,j+1)-xab*S(i,j);
      }
    }
  }

  // Return result, dropping the temporary 
  return S.submat(0,0,la,lb);
}

arma::mat kinetic_int_os(double xa, double ya, double za, double zetaa, const std::vector<shellf_t> & carta, double xb, double yb, double zb, double zetab, const std::vector<shellf_t> & cartb) {
  // Compute shell of kinetic energy integrals

  // Angular momenta of shells
  int am_a=carta[0].l+carta[0].m+carta[0].n;
  int am_b=cartb[0].l+cartb[0].m+cartb[0].n;

  // Returned matrix
  arma::mat T(carta.size(),cartb.size());

  // Get 1d overlap integrals
  arma::mat ox_arr=overlap_ints_1d(xa,xb,zetaa,zetab,am_a,am_b);
  arma::mat oy_arr=overlap_ints_1d(ya,yb,zetaa,zetab,am_a,am_b);
  arma::mat oz_arr=overlap_ints_1d(za,zb,zetaa,zetab,am_a,am_b);

  // Get kinetic energy integrals
  arma::mat kx_arr=derivative_ints_1d(xa,xb,zetaa,zetab,am_a,am_b,2);
  arma::mat ky_arr=derivative_ints_1d(ya,yb,zetaa,zetab,am_a,am_b,2);
  arma::mat kz_arr=derivative_ints_1d(za,zb,zetaa,zetab,am_a,am_b,2);

  double ox, oy, oz;
  double kx, ky, kz;

  int la, ma, na;
  int lb, mb, nb;

  double anorm, bnorm;

  for(size_t i=0;i<carta.size();i++) {
    anorm=carta[i].relnorm;

    la=carta[i].l;
    ma=carta[i].m;
    na=carta[i].n;

    for(size_t j=0;j<cartb.size();j++) {
      lb=cartb[j].l;
      mb=cartb[j].m;
      nb=cartb[j].n;

      bnorm=cartb[j].relnorm;

      ox=ox_arr(la,lb);
      oy=oy_arr(ma,mb);
      oz=oz_arr(na,nb);

      kx=kx_arr(la,lb);
      ky=ky_arr(ma,mb);
      kz=kz_arr(na,nb);

      T(i,j)=-0.5*anorm*bnorm*(kx*oy*oz + ox*ky*oz + ox*oy*kz);
    }
  }

#ifdef DEBUG      

  arma::mat huz(carta.size(),cartb.size());
  for(size_t i=0;i<carta.size();i++) {
    la=carta[i].l;
    ma=carta[i].m;
    na=carta[i].n;
    anorm=carta[i].relnorm;

    for(size_t j=0;j<cartb.size();j++) {
      lb=cartb[j].l;
      mb=cartb[j].m;
      nb=cartb[j].n;
      bnorm=cartb[j].relnorm;

      huz(i,j)=anorm*bnorm*kinetic_int(xa,ya,za,zetaa,la,ma,na,xb,yb,zb,zetab,lb,mb,nb);
    }
  }

  int diff=0;
  for(size_t i=0;i<carta.size();i++)
    for(size_t j=0;j<cartb.size();j++) 
      if(fabs(T(i,j)-huz(i,j))>10*DBL_EPSILON*fabs(huz(i,j)))
	diff++;

  if(diff==0) 
    printf("Computed shell of KE (%e,%e,%e)-(%e,%e,%e) with zeta=(%e,%e) and am=(%i,%i), the results match.\n",xa,ya,za,xb,yb,zb,zetaa,zetab,am_a,am_b);
  else
      for(size_t i=0;i<carta.size();i++)
	for(size_t j=0;j<cartb.size();j++) 
	  if(fabs(T(i,j)-huz(i,j))>1000*DBL_EPSILON*fabs(huz(i,j))) {
	    printf("Computed KE (%e,%e,%e)-(%e,%e,%e) with zeta=(%e,%e) and am=(%i,%i,%i)-(%i,%i,%i)\n",xa,ya,za,xb,yb,zb,zetaa,zetab,la,ma,na,lb,mb,nb);
	    printf("Huzinaga gives %e, OS gives %e.\n",huz(i,j),T(i,j));
	  }
 
#endif
      
  return T;
}

// Compute kinetic energy integral of unnormalized primitives at r_A and r_B
double kinetic_int_os(double xa, double ya, double za, double zetaa, int la, int ma, int na, double xb, double yb, double zb, double zetab, int lb, int mb, int nb) {

  // Kinetic energy contributions in x, y and z
  double kx, ky, kz;

  // Overlaps in x, y and z
  double ox, oy, oz;

  // Compute kinetic energy integrals
  //  kx=kinetic_int_1d(xa,xb,zetaa,zetab,la,lb);
  //  ky=kinetic_int_1d(ya,yb,zetaa,zetab,ma,mb);
  //  kz=kinetic_int_1d(za,zb,zetaa,zetab,na,nb);
  kx=derivative_int_1d(xa,xb,zetaa,zetab,la,lb,2);
  ky=derivative_int_1d(ya,yb,zetaa,zetab,ma,mb,2);
  kz=derivative_int_1d(za,zb,zetaa,zetab,na,nb,2);

  // Compute overlap integrals
  ox=overlap_int_1d(xa,xb,zetaa,zetab,la,lb);
  oy=overlap_int_1d(ya,yb,zetaa,zetab,ma,mb);
  oz=overlap_int_1d(za,zb,zetaa,zetab,na,nb);


#ifdef DEBUG
  // Check overlap
  double ov=ox*oy*oz;
  double hov=overlap_int(xa,ya,za,zetaa,la,ma,na,xb,yb,zb,zetab,lb,mb,nb);
  if(fabs(ov-hov)>10*DBL_EPSILON*fabs(hov)) {
    printf("Computed overlap integral (%e,%e,%e)-(%e,%e,%e) with zeta=(%e,%e) and am=(%i,%i,%i)-(%i,%i,%i)\n",xa,ya,za,xb,yb,zb,zetaa,zetab,la,ma,na,lb,mb,nb);
    printf("Huzinaga gives %e, OS gives %e.\n",hov,ov);
    printf("overlap\t%e\t%e\t%e\n\n",ox,oy,oz);
  }
#endif

  // The result is
  return -0.5*(kx*oy*oz + ox*ky*oz + ox*oy*kz);
}


// Worker function for kinetic energy
double kinetic_int_1d(double xa, double xb,double zetaa, double zetab, int la, int lb) {

  // In the following we assume la>lb.
  if(lb<la) // Switch arguments if necessary.
    return kinetic_int_1d(xb,xa,zetab,zetaa,lb,la);

  // Compute exponents
  double p=zetaa+zetab;

  // Compute center
  double px=(zetaa*xa+zetab*xb)/p;

  double xpa=px-xa;
  double xpb=px-xb;

  // Get overlap integrals
  arma::mat S=overlap_ints_1d(xa,xb,zetaa,zetab,la+1,lb);

  // Compute the kinetic energy integrals
  //      T_{ij} = - ½ <G_i | \partial_x^2 | G_j >
  // using recursion relations
  // T_{i+1,j} = X_{PA} T_{i,j} + 1/2p * ( i*T_{i-1,j} + j*T_{i,j-1} ) + zetab/p * (2 * zetaa * S_{i+1,j} - i*S_{i-1,j})
  // T_{i,j+1} = X_{PB} T_{i,j} + 1/2p * ( i*T_{i-1,j} + j*T_{i,j-1} ) + zetaa/p * (2 * zetab * S_{i,j+1} - j*S_{i,j-1})
  // and the initial result
  // T_{00} = zetaa * (1 - 2*zetaa*(X_{PA}^2 + 1/2p)) * S_{00}

  arma::mat T(la+2,lb+1);
  T.zeros();

  // Initialize array
  T(0,0)=zetaa*(1.0 - 2.0*zetaa*(xpa*xpa + 0.5/p))*S(0,0);

  // Generate integrals T_{i,0}
  T(1,0)=xpa*T(0,0) + zetab/p*2.0*zetaa*S(1,0);
  for(int i=1;i<=la;i++) {
    T(i+1,0)=xpa*T(i,0) + 0.5/p*i*T(i-1,0) + zetab/p*(2.0*zetaa*S(i+1,0)-i*S(i-1,0));
  }

  if(lb>0) {
    T(0,1)=xpb*T(0,0) + zetaa/p*2.0*zetab*S(0,1);
    for(int j=1;j<lb;j++)
      T(0,j+1)=xpb*T(0,j) + 0.5/p*j*T(0,j-1) + zetaa/p*(2.0*zetab*S(0,j+1)-j*S(0,j-1));

    // Form target integral T_{la,lb}
    for(int i=1;i<=la;i++)
      for(int j=1;j<lb;j++) {
	T(i,j+1)=xpb*T(i,j) + 0.5/p*(i*T(i-1,j) + j*T(i,j-1)) + zetaa/p*(2.0*zetab*S(i,j+1)-j*S(i,j-1));
      }
  }

  return T(la,lb);
}

// Compute matrix element of derivative
double derivative_int_1d(double xa, double xb, double zetaa, double zetab, int la, int lb, int eval) {

  return derivative_ints_1d(xa,xb,zetaa,zetab,la,lb,eval)(la,lb);

}

// Compute shell of matrix elements of derivative
arma::mat derivative_ints_1d(double xa, double xb, double zetaa, double zetab, int la, int lb, int eval) {

  // In the following we assume la>lb.
  if(lb<la) // Switch arguments if necessary.
    return trans(derivative_ints_1d(xb,xa,zetab,zetaa,lb,la,eval));

  // Work matrices
  std::vector<arma::mat> D;

  // We compute the wanted matrix with
  // D^{e+1}_{i,j} = 2*zetaa*D^e_{i+1,j} - iD^e_{i-1,j}

  // The lowest order matrix is simply the ovelap matrix,
  // which we need to get in a big enough form
  D.push_back(overlap_ints_1d(xa,xb,zetaa,zetab,la+eval,lb));
  
  // Now, perform the recursion.
  for(int e=1;e<=eval;e++) {
    // Number of rows in the matrix of the current iteration
    int laval=la+eval-e;

    // Create matrix
    D.push_back(arma::mat(laval+1,lb+1));
    D[e].zeros();

    // Do recursion
    for(int j=0;j<=lb;j++) // i=0
      D[e](0,j)=2*zetaa*D[e-1](1,j);
    for(int i=1;i<=laval;i++)
      for(int j=0;j<=lb;j++)
	D[e](i,j)=2*zetaa*D[e-1](i+1,j)-i*D[e-1](i-1,j);
  }

  // Return result
  return D[eval].submat(0,0,la,lb);
}

arma::mat nuclear_int_os(double xa, double ya, double za, double zetaa, const std::vector<shellf_t> & carta, double xnuc, double ynuc, double znuc, double xb, double yb, double zb, double zetab, const std::vector<shellf_t> & cartb) {
  // Compute shell of overlap integrals

  // Angular momenta of shells
  int am_a=carta[0].l+carta[0].m+carta[0].n;
  int am_b=cartb[0].l+cartb[0].m+cartb[0].n;

  // Compute the matrix
  arma::mat V=nuclear_ints_os(xa,ya,za,zetaa,am_a,xnuc,ynuc,znuc,xb,yb,zb,zetab,am_b);

  // Plug in the relative normalization factors
  for(size_t i=0;i<carta.size();i++)
    for(size_t j=0;j<cartb.size();j++)
      V(i,j)*=carta[i].relnorm*cartb[j].relnorm;

  return V;
}

double nuclear_int_os(double xa, double ya, double za, double zetaa, int la, int ma, int na, double xnuc, double ynuc, double znuc, double xb, double yb, double zb, double zetab, int lb, int mb, int nb) {

  // Compute the shell
  int am_a=la+ma+na;
  int am_b=lb+mb+nb;

  arma::mat ints=nuclear_ints_os(xa,ya,za,zetaa,am_a,xnuc,ynuc,znuc,xb,yb,zb,zetab,am_b);

  // Find the integral in the table
  double os=ints(getind(la,ma,na),getind(lb,mb,nb));

  return os;
}

/*

  The Obara-Saika recursion routine for nuclear attraction integrals
  was heavily inspired by the routine in

  PSI3: An open-source ab initio electronic structure package version 3.1.0.

  by

  T. D. Crawford, C. D. Sherrill, E. F. Valeev, J. T. Fermann, R. A. King,
  M. L. Leininger, S. T. Brown, C. L. Janssen, E. T. Seidl, J. P. Kenny,
  and W. D. Allen [J. Comp. Chem. 28, 1610 (2007)].

  The original license was the GNU General Public License.
  This version is relicensed under GPLv2+.

*/


arma::mat nuclear_ints_os(double xa, double ya, double za, double zetaa, int am_a, double xnuc, double ynuc, double znuc, double xb, double yb, double zb, double zetab, int am_b) {

  // Compute coordinates of center
  const double zeta=zetaa+zetab;
  const double o2g=1/(2.0*zeta);

  const double xp=(zetaa*xa+zetab*xb)/zeta;
  const double yp=(zetaa*ya+zetab*yb)/zeta;
  const double zp=(zetaa*za+zetab*zb)/zeta;

  const double PAx=xp-xa;
  const double PAy=yp-ya;
  const double PAz=zp-za;

  const double PBx=xp-xb;
  const double PBy=yp-yb;
  const double PBz=zp-zb;

  const double PCx=xp-xnuc;
  const double PCy=yp-ynuc;
  const double PCz=zp-znuc;

  const double ABsq=(xa-xb)*(xa-xb) + (ya-yb)*(ya-yb) + (za-zb)*(za-zb);

  // Sum of angular momenta
  const int mmax=am_a+am_b;

  const int size_a=(am_a+1)*(am_a+1)*am_a+1;
  const int size_b=(am_b+1)*(am_b+1)*am_b+1;

  // Work array for recursion formulas
  arma::cube ints(size_a,size_b,mmax+1);
  ints.zeros();

  // Helpers for computing indices on work array
  const int Nan = 1;
  const int Nam = am_a+1;
  const int Nal = Nam*Nam;
	
  const int Nbn = 1;
  const int Nbm = am_b+1;
  const int Nbl = Nbm*Nbm;

  // Argument of Boys' function
  const double boysarg=zeta*(PCx*PCx + PCy*PCy + PCz*PCz);
  // Evaluate Boys' function
  std::vector<double> bf=boysF_arr(mmax,boysarg);

  // Constant prefactor for auxiliary integrals
  const double prefac=2.0*M_PI/zeta*exp(-zetaa*zetab*ABsq/zeta);

  // Initialize integral array (0_A | A(0) | 0_B)^(m)
  for(size_t m=0;m<bf.size();m++)
    ints(0,0,m)=prefac*bf[m];

  // Increase angular momentum on right hand side.

  // Loop over total angular momentum
  for(int lambdab=1;lambdab<=am_b;lambdab++)
    
    // Loop over angular momentum functions belonging to this shell
    for(int ii=0; ii<=lambdab; ii++) {
      int lb=lambdab - ii;
      for(int jj=0; jj<=ii; jj++) {
	int mb=ii - jj;
	int nb=jj;
	
	// Index in integrals table
	int bind = lb*Nbl+mb*Nbm+nb*Nbn;
	
	if (nb > 0) {
	  for(int m=0;m<=mmax-lambdab;m++)
	    ints(0,bind,m) = PBz*ints(0,bind-Nbn,m)-PCz*ints(0,bind-Nbn,m+1);
	  
	  if (nb > 1) {
	    for(int m=0;m<=mmax-lambdab;m++)
	      ints(0,bind,m) += o2g*(nb-1)*(ints(0,bind-2*Nbn,m)-ints(0,bind-2*Nbn,m+1));
	  }
	}

	else if (mb > 0) {

	  for(int m=0;m<=mmax-lambdab;m++)
	    ints(0,bind,m) = PBy*ints(0,bind-Nbm,m)-PCy*ints(0,bind-Nbm,m+1);

	  if (mb > 1) {
	      for(int m=0;m<=mmax-lambdab;m++)  
		ints(0,bind,m) += o2g*(mb-1)*(ints(0,bind-2*Nbm,m)-ints(0,bind-2*Nbm,m+1));
	  }
	}
	
	else if (lb > 0) {

	  for(int m=0;m<=mmax-lambdab;m++)
	    ints(0,bind,m) = PBx*ints(0,bind-Nbl,m)-PCx*ints(0,bind-Nbl,m+1);

	  if (lb > 1) {
	    for(int m=0;m<=mmax-lambdab;m++)
	      ints(0,bind,m) += o2g*(lb-1)*(ints(0,bind-2*Nbl,m)-ints(0,bind-2*Nbl,m+1));
	  }

	}
	else {
	  ERROR_INFO();
	  throw std::runtime_error("Something went haywire in the Obara-Saika nuclear attraction integral algorithm.\n");
	}
      }
  }
  
  // Now, increase the angular momentum of the left-hand side, too.
  
  // Loop over total angular momentum of RHS
  for(int lambdab=0;lambdab<=am_b;lambdab++)

    // Loop over the functions belonging to the shell
    for(int ii=0; ii<=lambdab; ii++) {
      int lb=lambdab - ii;
      for(int jj=0; jj<=ii; jj++) {
	int mb=ii - jj;
	int nb=jj;
	
	// RHS index is
	int bind = lb*Nbl + mb*Nbm + nb*Nbn;
	
	// Loop over total angular momentum of LHS
	for(int lambdaa=1;lambdaa<=am_a;lambdaa++)
	  
	  // Loop over angular momentum of second shell
	  for(int kk=0; kk<=lambdaa; kk++) {
	    int la=lambdaa-kk;
	    
	    for(int ll=0; ll<=kk; ll++) {
	      int ma=kk-ll;
	      int na=ll;

	      // LHS index is
	      int aind = la*Nal + ma*Nam + na*Nan;

	      if (na > 0) {

		for(int m=0;m<=mmax-lambdaa-lambdab;m++) {
		  ints(aind,bind,m) = PAz*ints(aind-Nan,bind,m)-PCz*ints(aind-Nan,bind,m+1);
		}
		
		if (na > 1) {
		  for(int m=0;m<=mmax-lambdaa-lambdab;m++)
		    ints(aind,bind,m) += o2g*(na-1)*(ints(aind-2*Nan,bind,m)-ints(aind-2*Nan,bind,m+1));
		}
		
		if (nb > 0) {
		  for(int m=0;m<=mmax-lambdaa-lambdab;m++)
		    ints(aind,bind,m) += o2g*nb*(ints(aind-Nan,bind-Nbn,m)-ints(aind-Nan,bind-Nbn,m+1));
		}

	      } else if (ma > 0) {
		
		for(int m=0;m<=mmax-lambdaa-lambdab;m++) {
		  ints(aind,bind,m) = PAy*ints(aind-Nam,bind,m)-PCy*ints(aind-Nam,bind,m+1);
		}

		if (ma > 1) {
		  for(int m=0;m<=mmax-lambdaa-lambdab;m++)
		    ints(aind,bind,m) += o2g*(ma-1)*(ints(aind-2*Nam,bind,m) - ints(aind-2*Nam,bind,m+1));
		}
		
		if (mb > 0) {
		  for(int m=0;m<=mmax-lambdaa-lambdab;m++)
		    ints(aind,bind,m) += o2g*mb*(ints(aind-Nam,bind-Nbm,m) - ints(aind-Nam,bind-Nbm,m+1));
		}

	      }	else if (la > 0) {

		for(int m=0;m<=mmax-lambdaa-lambdab;m++) {
		  ints(aind,bind,m) = PAx*ints(aind-Nal,bind,m)-PCx*ints(aind-Nal,bind,m+1);
		}

		if (la > 1) {
		  for(int m=0;m<=mmax-lambdaa-lambdab;m++)
		    ints(aind,bind,m) += o2g*(la-1)*(ints(aind-2*Nal,bind,m) - ints(aind-2*Nal,bind,m+1));
		}

		if (lb > 0) {
		  for(int m=0;m<=mmax-lambdaa-lambdab;m++)
		    ints(aind,bind,m) += o2g*lb*(ints(aind-Nal,bind-Nbl,m) - ints(aind-Nal,bind-Nbl,m+1));  
		}
	      }
	      else {
		ERROR_INFO();
		throw std::runtime_error("Something went haywire in the Obara-Saika nuclear attraction integral algorithm.\n");
	      }
	    }
	  }
    }
  }

  // Size of returned array
  int Na=(am_a+1)*(am_a+2)/2;
  int Nb=(am_b+1)*(am_b+2)/2;

  // Returned array
  arma::mat V(Na,Nb);
  V.zeros();

  // Fill in array
  int ia, ib;

  // Index of left basis function
  ia=0;

  
  // Loop over basis functions on this shell
  for(int ii=0; ii<=am_a; ii++) {
    int la=am_a - ii;
    for(int jj=0; jj<=ii; jj++) {
      int ma=ii - jj;
      int na=jj;
      
      // Index in worker array
      int aind=la*Nal+ma*Nam+na;
      
      // Index of right basis function
      ib=0;
      
      // Loop over angular momentum of second shell
      for(int kk=0; kk<=am_b; kk++) {
	int lb=am_b - kk;
	
	for(int ll=0; ll<=kk; ll++) {
	  int mb=kk-ll;
	  int nb=ll;
	  
	  // Other index is
	  int bind=lb*Nbl+mb*Nbm+nb;
	  
	  // Store result
	  V(ia,ib)=-ints(aind,bind,0);
	  
	  // Increment index of basis function
	  ib++;
	}
      }
      
      // Increment index of basis function
      ia++;
    }
  }

#ifdef DEBUG


  // Compute Huzinaga integrals
  arma::mat huzint=V;
  huzint.zeros();
  
  int diff=0;

  ia=0;
  // Loop over basis functions on this shell
  for(int ii=0; ii<=am_a; ii++) {
    int ila=am_a - ii;
    for(int jj=0; jj<=ii; jj++) {
      int ima=ii - jj;
      int ina=jj;
      
      ib=0;
      // Loop over basis functions on this shell
      for(int kk=0; kk<=am_b; kk++) {
	int ilb=am_b - kk;
	for(int ll=0; ll<=kk; ll++) {
	  int imb=kk - ll;
	  int inb=ll;
	  
	  huzint(ia,ib)=nuclear_int(xa,ya,za,zetaa,ila,ima,ina,xnuc,ynuc,znuc,xb,yb,zb,zetab,ilb,imb,inb);

	  if(fabs(huzint(ia,ib)-V(ia,ib))>100*DBL_EPSILON*fabs(huzint(ia,ib)))
	    diff++;

	  ib++;
	}
      }
      
      ia++;
    }
  }

  if(diff==0)
    printf("Computed NAI shell (%e,%e,%e) - (%e,%e,%e) with nucleus at (%e,%e,%e) and exponents %e and %e with am=(%i,%i), the results match.\n",xa,ya,za,xb,yb,zb,xnuc,ynuc,znuc,zetaa,zetab,am_a,am_b);
  else {
    ia=0;
    // Loop over basis functions on this shell                                                                                                                                     
    for(int ii=0; ii<=am_a; ii++) {
      int ila=am_a - ii;
      for(int jj=0; jj<=ii; jj++) {
	int ima=ii - jj;
	int ina=jj;

	ib=0;
	// Loop over basis functions on this shell                                                                                                                                 
	for(int kk=0; kk<=am_b; kk++) {
	  int ilb=am_b - kk;
	  for(int ll=0; ll<=kk; ll++) {
	    int imb=kk - ll;
	    int inb=ll;

	    if(fabs(huzint(ia,ib)-V(ia,ib))>100*DBL_EPSILON*fabs(huzint(ia,ib))) {
	      printf("Computed NAI shell (%e,%e,%e) - (%e,%e,%e) with nucleus at (%e,%e,%e) and exponents %e and %e.\n",xa,ya,za,xb,yb,zb,xnuc,ynuc,znuc,zetaa,zetab);
	      printf("The result for the integral (%i,%i,%i)-(%i,%i,%i) is %e with Huzinaga, whereas %e with Obara-Saika.\n\n",ila,ima,ina,ilb,imb,inb,huzint(ia,ib),V(ia,ib));
	    }
	    ib++;
	  }
	}
	ia++;
      }
    }
  }
  
  /*    printf("Obara-Saika shell of integrals:\n");
	V.print();
	printf("Huzinaga shell of integrals:\n");
	huzint.print();
	printf("\n");*/
#endif
  
  return V;
}


arma::cube three_overlap_ints_os(double xa, double ya, double za, double xc, double yc, double zc, double xb, double yb, double zb, double zetaa, double zetac, double zetab, int am_a, int am_c, int am_b) {

  // Necessary size for work array
  const int size_a=(am_a+1)*(am_a+1)*am_a+1;
  const int size_c=(am_c+1)*(am_c+1)*am_c+1;
  const int size_b=(am_b+1)*(am_b+1)*am_b+1;

  // Work array for recursion formulas
  arma::cube ints(size_a,size_c,size_b);
  ints.zeros();

  // Helpers for computing indices on work array
  const int Nan = 1;
  const int Nam = am_a+1;
  const int Nal = Nam*Nam;
  
  const int Nbn = 1;
  const int Nbm = am_b+1;
  const int Nbl = Nbm*Nbm;

  const int Ncn = 1;
  const int Ncm = am_c+1;
  const int Ncl = Ncm*Ncm;

  // Reduced exponents
  double xi=zetaa*zetab/(zetaa+zetab);
  double zeta=zetaa+zetab;

  // r_ab
  double rabsq=(xa-xb)*(xa-xb)+(ya-yb)*(ya-yb)+(za-zb)*(za-zb);

  // P
  double px=(zetaa*xa+zetab*xb)/zeta;
  double py=(zetaa*ya+zetab*yb)/zeta;
  double pz=(zetaa*za+zetab*zb)/zeta;

  // r_pc
  double rpcsq=(px-xc)*(px-xc)+(py-yc)*(py-yc)+(pz-zc)*(pz-zc);

  // G
  double gx=(zeta*px+zetac*xc)/(zeta+zetac);
  double gy=(zeta*py+zetac*yc)/(zeta+zetac);
  double gz=(zeta*pz+zetac*zc)/(zeta+zetac);

  // GA
  double gax=gx-xa;
  double gay=gy-ya;
  double gaz=gz-za;
  // GB
  double gbx=gx-xb;
  double gby=gy-yb;
  double gbz=gz-zb;
  // GC
  double gcx=gx-xc;
  double gcy=gy-yc;
  double gcz=gz-zc;

  // Compute initial data.
  ints(0,0,0)=(M_PI/(zeta+zetac))*sqrt(M_PI/(zeta+zetac))*exp(-xi*rabsq - zeta*zetac/(zeta+zetac)*rpcsq);

  // Now am=(0,0,0). Increase LHS angular momentum.
  // Loop over total angular momentum
  for(int lambdaa=1;lambdaa<=am_a;lambdaa++)
    
    // Loop over angular momentum functions belonging to this shell
    for(int ii=0; ii<=lambdaa; ii++) {
      int la=lambdaa - ii;
      for(int jj=0; jj<=ii; jj++) {
	int ma=ii - jj;
	int na=jj;
	
	// Index in integrals table
	int aind = la*Nal+ma*Nam+na*Nan;
	
	// Compute integral
	if(la>0)
	  ints(aind,0,0)+=gax*ints(aind-Nal,0,0);
	if(la>1)
	  ints(aind,0,0)+=0.5/(zeta+zetac)*(la-1)*ints(aind-2*Nal,0,0);

	if(ma>0)
	  ints(aind,0,0)+=gay*ints(aind-Nam,0,0);
	if(ma>1)
	  ints(aind,0,0)+=0.5/(zeta+zetac)*(ma-1)*ints(aind-2*Nam,0,0);

	if(na>0)
	  ints(aind,0,0)+=gaz*ints(aind-Nan,0,0);
	if(na>1)
	  ints(aind,0,0)+=0.5/(zeta+zetac)*(na-1)*ints(aind-2*Nan,0,0);
      }
    }

  // Increase total angular momentum on right-hand side
  // Loop over total angular momentum of LHS
  for(int lambdaa=1;lambdaa<=am_a;lambdaa++)
    
    // Loop over angular momentum of second shell
    for(int kk=0; kk<=lambdaa; kk++) {
      int la=lambdaa-kk;
      
      for(int ll=0; ll<=kk; ll++) {
	int ma=kk-ll;
	int na=ll;
	
	// LHS index is
	int aind = la*Nal + ma*Nam + na*Nan;
	
	// Loop over total angular momentum of RHS
	for(int lambdab=0;lambdab<=am_b;lambdab++)
	  
	  // Loop over the functions belonging to the shell
	  for(int ii=0; ii<=lambdab; ii++) {
	    int lb=lambdab - ii;
	    for(int jj=0; jj<=ii; jj++) {
	      int mb=ii - jj;
	      int nb=jj;
	      
	      // RHS index is
	      int bind = lb*Nbl + mb*Nbm + nb*Nbn;

	      // Compute integral
	      if(lb>0) {

		ints(aind,0,bind)+=gbx*ints(aind,0,bind-Nbl);

		if(lb>1)
		  ints(aind,0,bind)+=0.5/(zeta+zetac)*(lb-1)*ints(aind,0,bind-2*Nbl);

		if(la>0)
		  ints(aind,0,bind)+=0.5/(zeta+zetac)*la*ints(aind-Nal,0,bind-Nbl);
	      } else if(mb>0) {

		ints(aind,0,bind)+=gby*ints(aind,0,bind-Nbm);
		if(mb>1)
		  ints(aind,0,bind)+=0.5/(zeta+zetac)*(mb-1)*ints(aind,0,bind-2*Nbm);
		if(ma>0)
		  ints(aind,0,bind)+=0.5/(zeta+zetac)*ma*ints(aind-Nam,0,bind-Nbm);
	      } else if(nb>0) {
		ints(aind,0,bind)+=gbz*ints(aind,0,bind-Nbn);
		if(nb>1)
		  ints(aind,0,bind)+=0.5/(zeta+zetac)*(nb-1)*ints(aind,0,bind-2*Nbn);	      
		if(na>0)
		  ints(aind,0,bind)+=0.5/(zeta+zetac)*na*ints(aind-Nan,0,bind-Nbn);
	      } else {
		ERROR_INFO();
		throw std::runtime_error("Something went haywire in the Obara-Saika three-center overlap algorithm.\n");
	      }
	    }
	  }
      }
    }
  
  // Finally, increase angular momentum in the middle
  // Loop over total angular momentum of LHS
  for(int lambdaa=1;lambdaa<=am_a;lambdaa++)
    
    // Loop over angular momentum of second shell
    for(int kk=0; kk<=lambdaa; kk++) {
      int la=lambdaa-kk;
      
      for(int ll=0; ll<=kk; ll++) {
	int ma=kk-ll;
	int na=ll;
	
	// LHS index is
	int aind = la*Nal + ma*Nam + na*Nan;
	
	// Loop over total angular momentum of RHS
	for(int lambdab=0;lambdab<=am_b;lambdab++)
	  
	  // Loop over the functions belonging to the shell
	  for(int ii=0; ii<=lambdab; ii++) {
	    int lb=lambdab - ii;
	    for(int jj=0; jj<=ii; jj++) {
	      int mb=ii - jj;
	      int nb=jj;
	      
	      // RHS index is
	      int bind = lb*Nbl + mb*Nbm + nb*Nbn;  
	      
	      // Loop over total angular momentum of middle
	      for(int lambdac=0;lambdac<=am_c;lambdac++)
		
		// Loop over the functions belonging to the shell
		for(int mm=0; mm<=lambdac; mm++) {
		  int lc=lambdac - mm;
		  for(int nn=0; nn<=mm; nn++) {
		    int mc=mm - nn;
		    int nc=nn;
	      
		    // RHS index is
		    int cind = lc*Ncl + mc*Ncm + nc*Ncn;  
		    
		    // Calculate integral
		    if(lc>0) {
		      ints(aind,cind,bind)+=gcx*ints(aind,cind-Ncl,bind);
	      
		      if(lc>1)
			ints(aind,cind,bind)+=0.5/(zeta+zetac)*(lc-1)*ints(aind,cind-2*Ncl,bind);
		      
		      if(la>0)
			ints(aind,bind,0)+=0.5/(zeta+zetac)*la*ints(aind-Nal,cind-Ncl,bind);
		      if(lb>0)
			ints(aind,bind,0)+=0.5/(zeta+zetac)*lb*ints(aind,cind-Ncl,bind-Nbl);
		    } else if(mc>0) {
		      ints(aind,cind,bind)+=gcy*ints(aind,cind-Ncm,bind);
	      
		      if(mc>1)
			ints(aind,cind,bind)+=0.5/(zeta+zetac)*(mc-1)*ints(aind,cind-2*Ncm,bind);
		      
		      if(ma>0)
			ints(aind,bind,0)+=0.5/(zeta+zetac)*ma*ints(aind-Nam,cind-Ncm,bind);
		      if(mb>0)
			ints(aind,bind,0)+=0.5/(zeta+zetac)*mb*ints(aind,cind-Ncm,bind-Nbm);
		    } else if(nc>0) {
		      ints(aind,cind,bind)+=gcz*ints(aind,cind-Ncn,bind);
	      
		      if(nc>1)
			ints(aind,cind,bind)+=0.5/(zeta+zetac)*(nc-1)*ints(aind,cind-2*Ncn,bind);
		      
		      if(na>0)
			ints(aind,bind,0)+=0.5/(zeta+zetac)*na*ints(aind-Nan,cind-Ncn,bind);
		      if(nb>0)
			ints(aind,bind,0)+=0.5/(zeta+zetac)*nb*ints(aind,cind-Ncn,bind-Nbn);
		    } else {
		      ERROR_INFO();
		      throw std::runtime_error("Something went haywire in the Obara-Saika three-center overlap algorithm.\n");
		    }
		  }
		}
	    }
	  }
      }
    }

  return ints;
}


