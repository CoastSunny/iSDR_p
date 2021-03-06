#include <cxxstd/iostream.h>
#include <flens/flens.cxx>
#include "matio.h"
#include <cmath>
#include <ctime>
#include <omp.h>
#include <vector>
#include "iSDR.h"
#include "MxNE.h"
#include "Matrix.h"
#include <algorithm>
#include "ReadWriteMat.h"
using namespace flens;
using namespace std;
using namespace Maths;
double Asum_x(double *a, double *b, int x){
    double a_x = 0.0;
    for (int i =0;i<x;i++)
        a_x += std::abs(a[i] - b[i]); 
    return a_x;
}
int test_iSDR_results(){
    // read data from mat file
    const char *file_path = "./examples/S1_p1_30.mat";
    int n_s_a = 12;
    int n_t_s = 220;
    Maths::DMatrix S(n_t_s, n_s_a);
    mat_t *matfp; // use matio to read the .mat file
    matvar_t *matvar;
    matfp = Mat_Open(file_path, MAT_ACC_RDONLY);
    if (matfp != NULL){
        matvar = Mat_VarRead(matfp, "S estimate") ;
        const double *xData = static_cast<const double*>(matvar->data);
        for(long unsigned int y = 0;y < (long unsigned int)n_s_a; ++y){
            for (long unsigned int x = 0; x < (long unsigned int)n_t_s; ++x)
                S.data()[x + y*n_t_s] = xData[x + y*n_t_s]; 
        }
    }
    else{
        std::cout<<"File not found ./examples/S1_p1_30.mat"<<std::endl;
        return 0;
    }
    file_path = "./examples/S1_p1_MEGT800.mat";
    int n_s=0, n_c=0, n_t=0, m_p=0;
    ReadWriteMat _RWMat(n_s, n_c, m_p, n_t);
    if (_RWMat.Read_parameters(file_path))
        return 0;
    n_s = _RWMat.n_s;
    n_c = _RWMat.n_c;
    m_p = _RWMat.m_p;
    n_t = _RWMat.n_t;
    n_t_s = n_t + m_p - 1;
    
    DMatrix G_o(n_c, n_s);
    DMatrix GA_initial(n_c, n_s*m_p);
    DMatrix M(n_c, n_t);
    IMatrix SC(n_s, n_s);
    bool use_mxne = true;
    DMatrix J(n_t_s, n_s);
    DMatrix Acoef(n_s, n_s*m_p);
    IVector Active(n_s);
    DVector Wt(n_s);
    if (_RWMat.ReadData(file_path, G_o, GA_initial, M, SC))
        return 0;
    double mvar_th = 1e-2;
    double alpha = 30;
    int n_mxne = 10000;
    int n_isdr = 100;
    double d_w_tol = 1e-7;
    iSDR _iSDR(n_s, n_c, m_p, n_t, alpha, n_mxne, n_isdr, d_w_tol, mvar_th, false);
    n_s = _iSDR.iSDR_solve(G_o, SC, M, GA_initial, J, Acoef, Active, Wt, use_mxne, false);
    DMatrix Jx(n_t_s, n_s);
    Underscore<Maths::DMatrix::IndexType> _;
    Jx = J(_(1, n_t_s), _(1, n_s));
    if (n_s != n_s_a)
        return 0;
    double x = Asum_x(&S.data()[0], &Jx.data()[0], n_s*n_t_s)/(n_s*n_t_s);
    if (x>1e-18)
        return 0;
    return 1;
}

int test_Compute_mu(){
    int n_s = 3; int n_c = 2; int m_p = 3; int n_t = 100; double d_w_tol=1e-6; 
    MxNE _MxNE(n_s, n_c, m_p, n_t, d_w_tol, false);
    Maths::DVector mu(n_s);
    Maths::DMatrix G(n_c, n_s*m_p);
    for(int j=0;j<n_s; ++j){
           for (int i =0; i<n_c; ++i){
               for(int k=0;k<m_p;k++)
                G(i+1,  j*m_p + 1 + k) = j+1;
           }
    }
    _MxNE.Compute_mu(G, mu);
    Maths::DVector mu_cp(n_s);
    mu_cp = 0.0833333,0.0208333,0.00925926;
    for (int i=0;i<n_s;i++){
        if (std::abs(mu(i+1)- mu_cp(i+1))>1e-4)
            return 0;
    }
    return 1;
}

int test_MxNE_absmax(){
    int n_s = 3; int n_c = 2; int m_p = 3; int n_t = 1; double d_w_tol=1e-6; 
    MxNE _MxNE(n_s, n_c, m_p, n_t, d_w_tol, false);
    Maths::DVector mu(n_s);
    mu = 0.0833333,0.0208333,0.00925926;
    double x = _MxNE.absmax(mu);
    if (std::abs(x - 0.0833333)>d_w_tol*1e-4)
        return 0;
    return 1;
}

int test_Compute_dX(){
    int n_s = 3; int n_c = 2; int m_p = 2; int n_t = 10; double d_w_tol=1e-6;
    int n_t_s = n_t + m_p -1; 
    MxNE _MxNE(n_s, n_c, m_p, n_t, d_w_tol, false);
    Maths::DMatrix G(n_c, n_s*m_p);Maths::DMatrix R(n_c, n_t);
    Maths::DMatrix X(n_t_s, n_s);Maths::DVector X_c(n_t_s);
    Maths::DVector X_e(n_t_s);
    std::fill(&X.data()[0], &X.data()[n_t_s], 1);
    for(int j=0;j<n_s; ++j){
           for (int i =0; i<n_c; ++i){
               for(int k=0;k<m_p;k++)
                G(i+1,  j*m_p + 1 + k) = j + 1;
           }
    }
    for (int l=0;l<m_p;l++){
        for (int i=0;i<n_t; i++){
            for (int j =0; j<n_c;j++){
                 for (int k=0;k<n_s;k++){
                    R(j+1, i+1) += G(j+1, k*m_p + 1 + l) * X(i+l+1, k+1);
                 }
            }
       }
    }
    //cxxblas::copy(n_c*n_t, &R.data()[0], 1, &_MxNE.R[0], 1);
    for (int i=0;i<n_s;i++){
        _MxNE.Compute_dX(G, R, X_c, i);
        std::fill(&X_e.data()[0], &X_e.data()[n_t_s], 8*(i+1));
        for (int j=0;j<m_p-1;j++){
             X_e(j+1) = 4*(i+1);
             X_e(n_t_s-j) = 4*(i+1);
        }
        double asum_x = Asum_x(&X_c.data()[0], &X_e.data()[0], n_t_s);
        if (asum_x != 0.0)
           return 0;
        std::fill(&X_c.data()[0], &X_c.data()[n_t_s], 0.0);
    }
    return 1; 
}
int test_Compute_GtR(){
    int n_s = 3; int n_c = 2; int m_p = 3; int n_t = 100; double d_w_tol=1e-6; 
    MxNE _MxNE(n_s, n_c, m_p, n_t, d_w_tol, false);
    Maths::DVector mu(n_s);
    Maths::DMatrix G(n_c, n_s*m_p);
    Maths::DMatrix R(n_c, n_t);
    Maths::DMatrix GtR(n_t, n_s*m_p);
    G = 0.5;
    R = 1;
    _MxNE.Compute_GtR(G, R, GtR);
    for (int j=0;j<m_p*n_s*n_t;j++){
        if (GtR.data()[j] != n_c*0.5)
            return 0;
    }
    return 1;
}

int test_Compute_alpha_max(){
    int n_s = 3; int n_c = 2; int m_p = 3; int n_t = 100; double d_w_tol=1e-6; 
    MxNE _MxNE(n_s, n_c, m_p, n_t, d_w_tol, false);
    Maths::DVector mu(n_s);
    Maths::DMatrix G(n_c, n_s*m_p);
    Maths::DMatrix R(n_c, n_t);
    G = 1;
    R = 1;
    double alpha_max = _MxNE.Compute_alpha_max(G, R);
    if (std::abs(alpha_max - n_c*std::sqrt(n_t*m_p)) > d_w_tol*1e-3)
            return 0;
    return 1;
}

int test_Compute_Me(){
    int n_s = 3; int n_c = 3; int m_p = 2; int n_t = 10; double d_w_tol=1e-6; 
    MxNE _MxNE(n_s, n_c, m_p, n_t, d_w_tol, false);
    Maths::DVector mu(n_s);
    Maths::DMatrix G(n_c, n_s*m_p);
    Maths::DMatrix M(n_c, n_t);
    Maths::DMatrix Me(n_c, n_t);
    Maths::DMatrix J(n_t+m_p-1, n_s);
    J = 1;
    M = m_p;
    for (int j=0;j<n_c;j++){
        G(j+1, j+1) = 1;
        G(j+1, n_c+j+1) = 1;
    }

    _MxNE.Compute_Me(G, J, Me);
    double x = Asum_x(&M.data()[0], &Me.data()[0], n_t*n_c);
    if (x > 0.0)
        return 0;
    return 1;
}

int test_update_r(){
    int n_s = 3; int n_c = 2; int m_p = 2; int n_t = 10; double d_w_tol=1e-6;
    int n_t_s = n_t + m_p -1; 
    MxNE _MxNE(n_s, n_c, m_p, n_t, d_w_tol, false);

    Maths::DVector X_i(n_t_s); Maths::DVector X_i_1(n_t_s); Maths::DVector X_i_2(n_t_s);
    Maths::DMatrix R(n_c, n_t); Maths::DMatrix G(n_c, n_s*m_p);
    for(int j=0;j<n_s; ++j){
           for (int i =0; i<n_c; ++i){
               for(int k=0;k<m_p;k++)
                G(i+1,  j*m_p + 1 + k) = 0.5*(j+1);
           }
    }
    std::fill(&X_i.data()[0], &X_i.data()[n_t_s], 1.0);
    std::fill(&X_i_2.data()[0], &X_i_2.data()[n_t_s], 2.0);
    //Maths::DMatrix R2(n_c, n_t);
    //cxxblas::copy(n_c*n_t, &R.data()[0], 1, &R2[0], 1);
    _MxNE.update_r(G, X_i, R, 0);
    _MxNE.Compute_dX(G, R, X_i_1, 0);
    X_i_2(1) = 1;X_i_2(n_t_s) = 1;
    if (Asum_x(&X_i_1.data()[0], &X_i_2.data()[0], n_t_s) != 0.0)
        return 0;
    return 1;
}


int test_duality_gap(){
    int n_s = 3; int n_c = 2; int m_p = 2; int n_t = 10; double d_w_tol=1e-6;
    int n_t_s = n_t + m_p -1; 
    MxNE _MxNE(n_s, n_c, m_p, n_t, d_w_tol, false);
    Maths::DMatrix M(n_c, n_t);Maths::DMatrix R(n_c, n_t);
    Maths::DMatrix G(n_c, n_s*m_p); Maths::DMatrix J(n_t_s, n_s);
    double alpha = 1e-3;
    for(int j=0;j<n_s; ++j){
        for (int i =0; i<n_c; ++i){
            for(int k=0;k<m_p;k++)
                G(i+1,  j*m_p + 1 + k) = 0.5*(j+1);
        }
    }
    std::fill(&J.data()[0], &J.data()[n_t_s], 1.0);
    for (int l=0;l<m_p;l++){
        for (int i=0;i<n_t; i++){
            for (int j =0; j<n_c;j++){
                for (int k=0;k<n_s;k++){
                    M(j+1, i+1) += G(j+1, k*m_p + 1 + l) * J(i+l+1, k+1);
                }
            }
       }
    }
    double gap;
    gap = _MxNE.duality_gap(G, M, J, R, alpha);
    if (gap!=alpha*std::sqrt(n_t_s))
        return 0;
    return 1;
}


int test_MxNE_solve(){
    int n_s = 3; int n_c = 3; int m_p = 1; int n_t = 10; double d_w_tol=1e-6; 
    MxNE _MxNE(n_s, n_c, m_p, n_t, d_w_tol, false);
    Maths::DVector mu(n_s);
    Maths::DMatrix G(n_c, n_s*m_p);
    Maths::DMatrix M(n_c, n_t);
    Maths::DMatrix Me(n_c, n_t);
    Maths::DMatrix J(n_t+m_p-1, n_s);
    Maths::DMatrix Jx(n_t+m_p-1, n_s);
    Jx = m_p;
    M = m_p;
    for (int j=0;j<n_c;j++)
        G(j+1, j+1) = 1;
    double alpha = 0.0001;
    int n_iter = 10000;
    double dual_gap_ = 0.0;
    double tol = 1e-10;
    bool initial = false;
    _MxNE.MxNE_solve(M, G, J, alpha, n_iter,dual_gap_,tol,initial);
    double x = Asum_x(&Jx.data()[0], &J.data()[0], n_s*(n_t+m_p-1))/(n_s*(n_t+m_p-1));
    if (x > 1e-3)
        return 0;
    initial = true;
    MxNE _MxNE2(n_s, n_c, m_p, n_t, d_w_tol, false);
    _MxNE2.MxNE_solve(M, G, J, alpha, n_iter,dual_gap_,tol,initial);
    x = Asum_x(&Jx.data()[0], &J.data()[0], n_s*(n_t+m_p-1))/(n_s*(n_t+m_p-1));
    if (x > 1e-3)
        return 0;
    return 1;
}

int test_Greorder(){
   int n_s=3;int n_c=2;int m_p=3;
   iSDR _iSDR(n_s, n_c, m_p, 100, 0.0, 1, 1, 1e-3, 0.001, false);
   Maths::DMatrix GA(n_c, n_s*m_p);Maths::DMatrix G_reorder(n_c, n_s*m_p);
   for (int k=0;k<m_p;++k){
       for(int j=0;j<n_s; ++j){
           for (int i =0; i<n_c; ++i)
                GA.data()[j*n_c + i + k*n_c*n_s] = j;
           }
       }
    _iSDR.Reorder_G(GA, G_reorder); 
    double asum_v[n_s];
    for (int i =0; i<n_s;i++){
        cxxblas::asum(n_c*m_p, &G_reorder.data()[i*n_c*m_p], 1, asum_v[i]);
        if (asum_v[i] != n_c*m_p*i)
           return  0;
    }   
    return 1;
}


int test_GxA(){
   int n_s=3;int n_c=2;int m_p=3;
   Maths::DMatrix G(n_c, n_s);Maths::DMatrix A(n_s, n_s*m_p);
   for(int j=0;j<n_s; ++j){
           for (int i =0; i<n_c; ++i)
                G(i+1,  j+1) = j;
   }
   for (int i =0; i<m_p;i++){
        for (int j =0; j<n_s;j++)
                A(j+1, j+1+i*n_s) = 1.0;
   }
   iSDR _iSDR(n_s, n_c, m_p, 100, 0.0, 1, 1, 1e-3, 0.001, false);
   Maths::DMatrix GA(n_c, n_s*m_p);
   _iSDR.G_times_A(G, A, GA);
   double asum_v[n_s];
   for (int i =0; i<n_s;i++){
       cxxblas::asum(n_c*m_p, &GA.data()[i*n_c*m_p], 1, asum_v[i]);
       if (asum_v[i] != n_c*m_p*i)
          return  0;
   }
   return 1;
}

int test_Reduce_G(){
   int n_s=5;int n_c=2;int m_p=3;
   Maths::DMatrix G(n_c, n_s);
    for(int j=0;j<n_s; ++j){
           for (int i =0; i<n_c; ++i)
                G(i+1,  j+1) = j;
    }
    iSDR _iSDR(n_s, n_c, m_p, 100, 0.0, 1, 1, 1e-3, 0.001, false);
    std::vector<int> ind;int x = 1;int y = 3;
    ind.push_back(x);ind.push_back(y);
    Maths::DMatrix G_n(n_c, ind.size());
    _iSDR.Reduce_G(G.data(), G_n, ind);
    for (unsigned int i=0;i<ind.size();i++){
        for (int j=0;j<n_c;j++){
            if (G_n(j+1, i+1) != G(j+1, ind[i]+1))
               return 0;
        }
    }
    return 1;
}

int test_Reduce_SC(){
   int n_s=6;int n_c=2;int m_p=3;
   Maths::IMatrix SC(n_s, n_s);
   iSDR _iSDR(n_s, n_c, m_p, 100, 0.0, 1, 1, 1e-3, 0.001, false);
   std::vector<int> ind;
   int x = 1;int y = 3;int z = 5;
   ind.push_back(x-1);ind.push_back(y-1);ind.push_back(z-1);
   for (int i=0;i<n_s;i++)
       SC(i+1, i+1) = 1;
   SC(y, x) = 1;SC(z, x) = 1;Maths::IMatrix SC_n(3, 3);
   _iSDR.Reduce_SC(SC.data(), SC_n, ind);
   for (int i=0; i<3;i++){
       for (int j=0;j<3;j++){
           if (SC_n(i+1, j+1) != SC(ind[i]+1, ind[j]+1))
              return 0;       
        }
   }
   return 1;
}

int test_A_step_lsq(){
    int n_s=3;int n_c=2;int m_p=2;int n_t = 100;double acceptable = 1e-2;
    typedef GeMatrix<FullStorage<double, ColMajor> > GeMatrix1;
    typedef GeMatrix<FullStorage<int, ColMajor> > GeMatrix2;
    typedef typename Maths::DMatrix::IndexType     IndexType;
    const Underscore<IndexType>  _;
    GeMatrix1 SC(n_s, n_s);
    int n_t_s = n_t;
    iSDR _iSDR(n_s, n_c, m_p, n_t, 0.0, 1, 1, 1e-3, 0.001, false);
    _iSDR.n_t_s = n_t_s;
    GeMatrix1 A(n_s, n_s*m_p);
    A(1,1) = 0.47599234;A(1,2) = 0.12215775;A(2,1) = -0.12460725;
    A(2,2) = 0.46669897;A(1,1+n_s) = 0.43268679;A(1,2+n_s) = 0.24245205;
    A(2,1+n_s) = -0.24120229;A(2,2+n_s) = 0.42778779;A(3,3+n_s) = 1.0;
    double Ax[2];
    Ax[0] = 0.47599234;
    Ax[1] = 0.46669897;
    GeMatrix1 S(n_t_s, n_s);
    S(1,1) = 6.15700359;S(2,1) = 5;S(1,2) = -3.64289379;S(2,2) = -5;
    S(1, 3) = 1.0;S(2, 3) = 1.0;
    for (int i=m_p+1;i<=n_t_s;i++){
        for (int j=0;j<n_s;j++){
             for (int k=0;k<n_s;k++){
                 for (int l=0;l<m_p;l++)
                    S(i, j+1) += A(j+1, k+1 + n_s*l)*S(i-m_p+l, k+1); 
            }
        }
    }
    GeMatrix2 A_scon(n_s, n_s);
    for (int i=1;i<=n_s;i++)
        A_scon(i, i) = 1;
    A_scon(1, 2) = 1;A_scon(2, 1) = 1;
    GeMatrix1 VAR(n_s, n_s*m_p);
    double Wt[n_s];
    _iSDR.A_step_lsq(&S.data()[0], &A_scon.data()[0], 1e-3, &VAR.data()[0], &Wt[0]);
    VAR(1, _(1, n_s*m_p)) *= Ax[0];
    VAR(2, _(1, n_s*m_p)) *= Ax[1];
    GeMatrix1 Se(n_t_s, n_s);
    Se(1,1) = 6.15700359;Se(2,1) = 5;Se(1,2) = -3.64289379;Se(2,2) = -5;
    Se(1, 3) = 1.0;Se(2, 3) = 1.0;
    for (int i=m_p+1;i<=n_t_s;i++){
        for (int j=0;j<n_s;j++){
             for (int k=0;k<n_s;k++){
                 for (int l=0;l<m_p;l++)
                        Se(i, j+1) += VAR(j+1, k+1 + n_s*l)*Se(i-m_p+l, k+1); 
            }
        }
    }
    double asum_s = Asum_x(&Se.data()[0],&S.data()[0], n_s*n_t_s)/(n_s*n_t_s);
    double asum_a = Asum_x(&A.data()[0],&VAR.data()[0], n_s*n_s*m_p)/(n_s*n_s*m_p);
    if (asum_s > acceptable || asum_a>acceptable){
        std::cout<<asum_s<<" "<<asum_a<<std::endl;
        return 0;
    }
    return 1;
    }

int test_Zero_non_zero(){
    std::vector<int> ind;std::vector<int> ind_c;
    int n_s=30;int n_c=2;int m_p=2;int n_t = 300;
    Maths::DMatrix S(n_t, n_s);
    iSDR _iSDR(n_s, n_c, m_p, n_t-m_p+1, 0.0, 1, 1, 1e-3, 0.001, false);
    ind = _iSDR.Zero_non_zero(S);
    if (ind.size()>0)
        return 0;
    int x = 10;int y = 20;
    S(x,x) = 1.0;S(y,y) = 1.0;
    ind_c.push_back(x-1);ind_c.push_back(y-1);
    ind = _iSDR.Zero_non_zero(S);
    if (ind.size()!=2)
        return 0;
    for (unsigned int i = 0; i<ind.size();i++){
        if (ind_c[i]!=ind[i])
           return 0;    
    }
        
    return 1;
}

int test_iSDR_solve(){
    int n_s = 3; int n_c = 3; int m_p = 1; int n_t = 50; double d_w_tol=1e-6; 
    MxNE _MxNE(n_s, n_c, m_p, n_t, d_w_tol, false);
    Maths::DVector mu(n_s);
    Maths::DMatrix G(n_c, n_s*m_p);
    Maths::IMatrix SC(n_s, n_s);
    Maths::DMatrix M(n_c, n_t);
    Maths::DMatrix Me(n_c, n_t);
    Maths::DMatrix J(n_t+m_p-1, n_s);
    Maths::DMatrix Jx(n_t+m_p-1, n_s);
    Jx = m_p;
    M = m_p;
    for (int j=0;j<n_c;j++){
        G(j+1, j+1) = 1;
        SC(j+1, j+1) = 1;
    }
    int n_iter = 10000;
    double dual_gap_ = 0.0;
    double tol = 1e-10;
    bool initial = false;
    double alpha_max = _MxNE.Compute_alpha_max(G, M);
    double a_per = 0.01;
    double alpha = alpha_max*a_per/100.0;
    _MxNE.MxNE_solve(M, G, J, alpha, n_iter,dual_gap_,tol,initial);
    double x = Asum_x(&Jx.data()[0], &J.data()[0], n_s*(n_t+m_p-1))/(n_s*(n_t+m_p-1));
    if (x > 1e-2)
        return 0;
    iSDR _iSDR(n_s, n_c, m_p, n_t, a_per, n_iter, 1, d_w_tol, 1e-6, false);
    Maths::DMatrix Jisdr(n_t+m_p-1, n_s);
    Maths::DMatrix Acoef(n_s, n_s);
    Maths::IVector Active(n_s);
    Maths::DVector Wt(n_s);
    int nsx = _iSDR.iSDR_solve(G, SC, M, G, Jisdr, Acoef, Active, Wt, false, false);
    x = Asum_x(&Jx.data()[0], &Jisdr.data()[0], n_s*(n_t+m_p-1))/(n_s*(n_t+m_p-1));
    if (x > 1e-2 || nsx !=n_s)
        return 0;
    x = Asum_x(&J.data()[0], &Jisdr.data()[0], n_s*(n_t+m_p-1))/(n_s*(n_t+m_p-1));
    if (x > 0)
        return 0;
    iSDR ziSDR(n_s, n_c, m_p, n_t, 100, n_iter, 5, tol, 1e-6, false);
    Jisdr=0;
    nsx = ziSDR.iSDR_solve(G, SC, M, G, Jisdr, Acoef, Active, Wt, false, false);
    if (nsx > 0)
        return 0;
    return 1; 
}

int test_Depth_Comp(){
    int n_s = 3; int n_c = 2; int m_p = 3; int n_t = 100; double d_w_tol=1e-6; 
    iSDR _iSDR(n_s, n_c, m_p, n_t, 0.0, 1, 1, d_w_tol, 0.001, false);
    Maths::DMatrix GA(n_c, n_s*m_p);
    for(int j=0;j<n_s; ++j){
           for (int i =0; i<n_c; ++i){
               for(int k=0;k<m_p;k++)
                GA(i+1,  j*m_p + 1 + k) = j*2.0+1.0;
           }
    }
    _iSDR.Depth_comp(GA);
    for (int i=0;i<n_s;i++){
        double n;
        cxxblas::nrm2(n_c*m_p, &GA.data()[i*n_c*m_p], 1, n);
        if ( std::abs(n-1.0) > 1e-3)
            return 0;
    }
    return 1;
}

int test_GA_removeDC(){
    int n_s = 2; int n_c = 2; int m_p = 2; int n_t = 100; double d_w_tol=1e-6; 
    iSDR _iSDR(n_s, n_c, m_p, n_t, 0.0, 1, 1, d_w_tol, 0.001, false);
    Maths::DMatrix A(n_s, n_s*m_p);
    A = 0.432, 0.242 , 0.476, 0.122,
        -0.241, 0.427,-0.124, 0.466;
    _iSDR.GA_removeDC(A);
    for (int i=0;i<n_s;++i){
        double x = 0.0;
        for (int j=0;j<n_c*m_p;++j){
            x += A.data()[i*n_c*m_p+j];
        }
        x /= (n_c*m_p);
        if (x > d_w_tol*1e-3)
            return 0;
    }
    return 1;
}

int test_Phi_TransitionMatrix(){
    int n_s = 2; int n_c = 2; int m_p = 2; int n_t = 100; double d_w_tol=1e-6; 
    iSDR _iSDR(n_s, n_c, m_p, n_t, 0.0, 1, 1, d_w_tol, 0.001, false);
    Maths::DMatrix A(n_s, n_s*m_p);
    A = 0.432, 0.242 , 0.476, 0.122,
        -0.241, 0.427,-0.124, 0.466;
    double EigMax=0;
    EigMax=_iSDR.Phi_TransitionMatrix(A);
    if (std::abs(EigMax-0.98634)> 1e-3)
        return 0;
    A *= 1/(EigMax*EigMax);
    EigMax=_iSDR.Phi_TransitionMatrix(A);
    if (std::abs(EigMax-1.00454) > 1e-3)
        return 0;
    return 1;
}

int test_ReadParameter(){
    int n_c = 306;
    int n_s = 562;
    int m_p = 3;
    int n_t = 294;
    int n_t_s = n_t + m_p-1;
    const char *file_path = "./examples/S1_data_p3.mat";
    ReadWriteMat _RWMat(n_s, n_c, m_p, n_t);
    if (_RWMat.Read_parameters(file_path))
        return 0;

    if (n_s - _RWMat.n_s + n_c - _RWMat.n_c + m_p - _RWMat.m_p +
     n_t - _RWMat.n_t + n_t_s - _RWMat.n_t_s != 0)
        return 0;
    return 1;
}

int test_ReadData(){
    int n_c = 306;
    int n_s = 562;
    int m_p = 3;
    int n_t = 294;
    const char *file_path = "./examples/S1_data_p3.mat";
    ReadWriteMat _RWMat(n_s, n_c, m_p, n_t);
    if (_RWMat.Read_parameters(file_path))
        return 0;
    Maths::DMatrix G_o(n_c, n_s);
    Maths::DMatrix GA(n_c, n_s*m_p);
    Maths::DMatrix R(n_c, n_t);
    Maths::IMatrix SC(n_s,  n_s);
    if(_RWMat.ReadData(file_path, G_o, GA, R, SC))
        return 0;
    return 1;
}
int main(){
    if (test_Compute_mu())
        printf( "MxNE.Compute_mu    ... Ok\n");
    else
        printf( "MxNE.Compute_mu    ... Failed\n");
    if (test_MxNE_absmax())
        printf( "MxNE.absmax        ... Ok\n");
    else
        printf( "MxNE.absmax    ... Failed\n");
        
    if (test_Compute_dX())
        printf( "MxNE.Compute_dX    ... Ok\n");
    else
        printf( "MxNE.Compute_dX    ... Failed\n");
    if (test_Compute_GtR())
        printf( "MxNE.Compute_GtR   ... Ok\n");
    else
        printf( "MxNE.Compute_GtR   ... Failed\n");
    if (test_Compute_alpha_max())
        printf( "MxNE.alpha_max     ... Ok\n");
    else
        printf( "MxNE.alpha_max   ... Failed\n");
    if (test_Compute_Me())
        printf( "MxNE.Compute_Me    ... Ok\n");
    else
        printf( "MxNE.Compute_Me   ... Failed\n");
    if (test_update_r())
        printf( "MxNE.update_r      ... Ok\n");
    else
        printf( "MxNE.update_r      ... Failed\n");
    if (test_duality_gap())
        printf( "MxNE.duality_gap   ... Ok\n");
    else
        printf( "MxNE.duality_gap   ... Failed\n");
    if (test_MxNE_solve())
        printf( "MxNE.MxNE_solve    ... Ok\n");
    else
        printf( "MxNE.MxNE_solve   ... Failed\n");
    if (test_Greorder())
        printf( "iSDR.Reorder_G     ... Ok\n");
    else
        printf( "iSDR.Reorder_G     ... Failed\n");

    if (test_GxA())
        printf( "iSDR.G_times_A     ... Ok\n");
    else
        printf( "iSDR.G_times_A     ... Failed\n");
    if (test_Reduce_G()) 
        printf( "iSDR.Reduce_G      ... Ok\n");
    else
        printf( "iSDR.Reduce_G      ... Failed\n");
    if (test_Reduce_SC())
         printf( "iSDR.Reduce_SC     ... Ok\n");
    else
        printf( "iSDR.Reduce_SC     ... Failed\n");
    
    if (test_A_step_lsq())
         printf( "iSDR.A_step_lsq    ... Ok\n");
    else
        printf( "iSDR.A_step_lsq    ... Failed\n");
    if (test_Zero_non_zero())
         printf( "iSDR.Zero_non_zero ... Ok\n");
    else
        printf( "iSDR.Zero_non_zero ... Failed\n");
    if (test_Depth_Comp())
         printf( "iSDR.Depth_comp    ... Ok\n");
    else
        printf( "iSDR.Depth_Comp    ... Failed\n");
    if (test_Phi_TransitionMatrix())
        printf( "iSDR.Phi_TraMatrix ... Ok\n");
    else
        printf( "iSDR.Phi_TraMatrix... Failed\n");
    if (test_GA_removeDC())
        printf( "iSDR.GA_removeDC   ... Ok\n");
    else
        printf( "iSDR.GA_removeDC... Failed\n");
    if (test_iSDR_solve())
        printf( "iSDR.iSDR_solve    ... Ok\n");
    else
        printf( "iSDR.iSDR_solve    ... Failed\n");
    if (test_iSDR_results())
        printf( "iSDR.iSDR_results  ... Ok\n");
    else
        printf( "iSDR.iSDR_results    ... Failed\n");
    if (test_ReadParameter())
        printf( "ReadParameter      ... Ok\n");
    else
        printf( "ReadParameter      ... Failed\n");
    if (test_ReadData())
        printf( "ReadData           ... Ok\n");
    else 
        printf( "ReadData          ... Failed\n");
    return 0; 
}
