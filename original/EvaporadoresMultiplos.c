//-----------------------------------------------------------------------------
/*
   Universidade Federal de Uberlândia
   Faculdade de Engenharia Química

Data: 23/03/2026
Autor: Pedro Paulo Nogueira Mendonça

Resolve um sistema de evaporadores de n
múltiplos efeitos COM EPE.

Resolve o sistema linear gerado a partir
dos balanços via Método de Gauss-Seidel.
*/
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
/*
   PROBLEMA A SER RESOLVIDO:

   Deseja-se projetar um sistema de evaporadores de triplo efeito com alimentação
direta para concentrar uma solução aquosa de Hidródixo de Sódio NaOH de uma
concentração inicial de 10% até uma concentração final de 50% (em peso).

    A alimentação entra no primeiro efeito a uma taxa de 15.000 kg/h e a uma temperatura
de 50°C. O vapor de aquecimento (vapor vivo) é fornecido ao primeiro efeito como vapor
saturado a 140°C. O último efeito opera a uma pressão onde a temperatura de saturação do
vapor é de 50°C.

    Considere áreas iguais nos três efeitos: A1 = A2 = A3 = A.
*/
//-----------------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

//-----------------------------------------------------------------------------

typedef struct {
  double *V;
  double *L;
  double *T;     // Temperatura do evaporador n [ºC]
  double *T_sat; // Temperatura de saturação sem EPE [ºC]
  double *dT;    // Delta T em cada evaporador
  double *x;     // Concentração do condensado no evaporador n [%]
  double *U;     // Coeficientes globais de TC para cada evaporador
  double *A;     // Área de troca térmica do evaporador n
  double *EPE;   // Elevação do ponto de ebulição em cada evaporador [ºC]
  double *P;     // Pressão em cada evaporador [Pa]
} StateVec;

typedef struct {
  int n;
  int ItTotal;
  int ItSistema;
  double S;
  double TS;
  double TF;
  double xF;
  double F;
  double DT;
  double omega;
} params;

params *par; // Informação global

//-----------------------------------------------------------------------------

#define TOL 1e-10 // Tolerância do erro de cálculo / iteração

StateVec *SetDomain();
StateVec *sv_alloc();
void Solver(StateVec *sv);
void freeVec(StateVec *sv);
void SolveSistema(StateVec *sv);
void printResults(StateVec *sv);
void AvaliarEstabilidade(StateVec *sv);
double HL(double T, double x);
double dHvap(double T);
double Hvsat(double T);
double h_super(double T, double P);
double Psat(double T);
double Tsat(double P);
double calc_T(double P, double x);

//-----------------------------------------------------------------------------

int main()
{

    par = calloc(1, sizeof(params)); // Paprâmetros globais

    par->n = 8; // Número de efeitos

    printf("omega  ;  ItSistema  ;  ItArea  ");

    for (int i=1; i<=1000; i++){

        par->omega = 0.001*i;
        par->ItSistema = 0.;
        par->ItTotal = 0.;

        // Declaração das variáveis
        StateVec *sv;
        sv = SetDomain();

        Solver(sv);

        //printResults(sv);
        printf("\n%.4f  ;  %d  ;  %d ", par->omega, par->ItSistema, par->ItTotal);

        freeVec(sv);
    }

    return 0;
}

//-----------------------------------------------------------------------------

StateVec *SetDomain() { // Definição de variáveis globais

  StateVec *sv;
  sv = sv_alloc();

    sv->U[0] = 2000.;                               // [W/(m^2 K)]
    for(int i=1; i<par->n-1; i++) sv->U[i] = 2000.; // [W/(m^2 K)]
    sv->U[par->n-1] = 2000.;                        // [W/(m^2 K)]

    // ENTRADA
    par->TS = 160.;  // [ºC]
    par->TF = 50.;   // [ºC]
    par->xF = 0.1;
    par->F = 10000.; // Entrada [kg/h]

    // SAÍDA - EVAPORADOR n
    sv->x[par->n-1] = 0.5;
    sv->L[par->n-1] = par->xF/sv->x[par->n-1]*par->F; // Saída [kg/h]
    sv->T_sat[par->n-1] = 60.;                        // [ºC]
    sv->P[par->n-1] = Psat(sv->T_sat[par->n-1]);      // [kPa]

    // Inicializando variáveis
    double sumEPE = 0.;

    for(int i=0; i<par->n; i++) sv->V[i] = ( par->F - sv->L[par->n-1] )/ par->n; // Tomando V[i] = V[i+1] = ...
    sv->L[0] = par->F - sv->V[0];
    for(int i=1; i<par->n-1; i++) sv->L[i] = sv->L[i-1] - sv->V[i];
    for(int i=0; i<par->n-1; i++) sv->x[i] = par->xF * par->F / sv->L[i];

    // Inicializando a pressão em cada evaporador
    double P1 = Psat(par->TS);                     // pressão do vapor vivo (~140°C)
    double Pn = Psat(sv->T_sat[par->n - 1]);       // pressão do último efeito (~50°C)

    for(int i = 0; i < par->n; i++){
             sv->P[i] = P1 - i * (P1 - Pn) / (par->n - 1);
    }

    for(int i=0; i<par->n; i++)   sv->T[i] = calc_T(sv->P[i], sv->x[i]);
    for(int i=0; i<par->n-1; i++) sv->T_sat[i] = Tsat(sv->P[i]);

    for(int i=0; i<par->n; i++) sv->EPE[i] = sv->T[i] - sv->T_sat[i];

    for(int i=0; i<par->n-1; i++) sumEPE += sv->EPE[i];
    par->DT = par->TS - sv->T_sat[par->n-1] - sumEPE; // Delta T TOTAL do sistema

    //printf("\nVazao de saida de concentrado L[%d] = %f kg/h", par->n, sv->L[par->n-1]);
    //printf("\nDelta T total da associacao = %f *C", par->DT);
    //printf("\nEPE total da associacao = %f *C", sumEPE);

    AvaliarEstabilidade(sv);

    return sv;

}

//-----------------------------------------------------------------------------

void Solver(StateVec *sv){

    // Estimativas iniciais
    double errorIt;
    double sum1;
    double sum2;
    double sumEPE;

    do{
        errorIt = 0.;
        sum1 = 0.;
        sum2 = 0.;
        sumEPE = 0.;

        sv->dT[0] = par->TS - sv->T[0];
        for (int i=1; i<par->n; i++) sv->dT[i] = sv->T_sat[i-1] - sv->T[i];

        AvaliarEstabilidade(sv);

        SolveSistema(sv);

        sv->A[0] = ( par->S * dHvap(par->TS) )/( sv->U[0] * sv->dT[0] * 3.6 );

        for(int i=1; i<par->n; i++) sv->A[i] = ( sv->V[i-1]*dHvap(sv->T_sat[i-1]))/(sv->U[i]*sv->dT[i]*3.6);

        errorIt = fmax(errorIt , fabs(sv->A[0] - sv->A[par->n-1]));

        for(int i=1; i<par->n; i++) errorIt = fmax(errorIt , fabs(sv->A[i] - sv->A[i-1]));

        for(int i=0; i<par->n; i++) sum1 += sv->A[i] * sv->dT[i];
        for(int i=0; i<par->n; i++) sum2 += sv->dT[i];
        double Am = sum1 / sum2;

        // Correção nos valores
        for(int i=0; i<par->n; i++)   sv->dT[i]    = sv->dT[i] * sv->A[i] / Am;
        for(int i=0; i<par->n-1; i++) sv->x[i]     = par->xF * par->F / sv->L[i];
        for(int i=0; i<par->n-1; i++) sv->T_sat[i] = sv->T[i+1] + sv->dT[i+1];
        for(int i=0; i<par->n-1; i++) sv->P[i]     = Psat(sv->T_sat[i]);
        for(int i=0; i<par->n-1; i++) sv->T[i]     = calc_T(sv->P[i], sv->x[i]);
        for(int i=0; i<par->n-1; i++) sv->EPE[i]   = sv->T[i] - sv->T_sat[i];

        for(int i=0; i<par->n-1; i++) sumEPE += sv->EPE[i];
        par->DT = par->TS - sv->T[par->n-1] - sumEPE; // Delta T TOTAL do sistema

        //printf("\nDelta T total da associacao = %f *C", par->DT);
        //printf("\nEPE total da associacao = %f *C", sumEPE);

        par->ItTotal = par->ItTotal + 1;

    } while(errorIt > TOL);

}

//-----------------------------------------------------------------------------

void SolveSistema(StateVec *sv){

    // Algoritimo de resolução Gauss-Seidel-SOR
    double errorIt;

    do{
        errorIt = 0.;
        double sum = 0;
        double S_old = par->S;
        double V_old[par->n];
        double L_old[par->n];
        for(int i=0; i<par->n; i++) V_old[i] = sv->V[i];
        for(int i=0; i<par->n; i++) L_old[i] = sv->L[i];

        sv->V[0] = ( sv->V[1]*h_super(sv->T[1],sv->P[1]) + sv->L[1]*HL(sv->T[1], sv->x[1]) - sv->L[0]*HL(sv->T[0], sv->x[0]) )/( dHvap(sv->T[0]) );

        sv->L[0] = par->F - sv->V[0];

        for(int i=1; i<par->n-1; i++){
            sv->V[i] = ( sv->V[i+1]*h_super(sv->T[i+1],sv->P[i+1]) + sv->L[i+1]*HL(sv->T[i+1], sv->x[i+1]) - sv->L[i]*HL(sv->T[i], sv->x[i]) )/( dHvap(sv->T[i]) );
            sv->L[i] = sv->L[i-1] - sv->V[i];
        }

        sv->V[par->n-1] = sv->L[par->n-2] - sv->L[par->n-1];

        par->S = ( sv->V[0]*h_super(sv->T[0],sv->P[0]) + sv->L[0]*HL(sv->T[0], sv->x[0]) - par->F*HL(par->TF, par->xF) )/( dHvap(par->TS) );


        par->S = S_old + par->omega * ( par->S - S_old);
        sv->V[0] = V_old[0] + par->omega * ( sv->V[0] - V_old[0]);
        sv->L[0] = L_old[0] + par->omega * ( sv->L[0] - L_old[0]);
        for(int i=1; i<par->n-1; i++){
            sv->V[i] = V_old[i] + par->omega * ( sv->V[i] - V_old[i]);
            sv->L[i] = L_old[i] + par->omega * ( sv->L[i] - L_old[i]);
        }
        sv->V[par->n-1] = V_old[par->n-1] + par->omega * ( sv->V[par->n-1] - V_old[par->n-1]);


        for(int i=0; i<par->n; i++) sum += pow((sv->V[i] - V_old[i])/V_old[i], 2) + pow((sv->L[i] - L_old[i])/L_old[i], 2);

        errorIt = pow((par->S - S_old)/S_old, 2) + sum;

        for(int i=0; i<par->n-1; i++) sv->x[i] = par->xF * par->F / sv->L[i];

        par->ItSistema = par->ItSistema + 1;

    }while(errorIt > TOL);

}

//-----------------------------------------------------------------------------

double HL(double T, double x){

    // Temperatura em Farenheit

    double Tf = 32.0 + 9.0 * T / 5.0; // [ºF]

    // Entalpia da solução de NaOH (R² = 0,99919)

    double z0 = -26.01021;
    double a  = -207.89085;
    double b  = 0.99923;
    double c  = 795.64522;
    double d  = -1.27127e-5;
    double f  = -0.47635;

    double H = z0 + a*x + b*Tf + c*pow(x, 2) + d*pow(Tf, 2) + f*x*Tf; // [BTU/lbm]

    return 2.326 * H; // [kJ/kg]
}

//-----------------------------------------------------------------------------

double dHvap(double T){

    // Correlação de Watson
    double a = (1. - (T+273.15)/647.096)/(1. - 373.15/647.096);
    return 2256.47 * pow(a, 0.38); // [kJ/kg]
}

//-----------------------------------------------------------------------------

double Hvsat(double T){

    // Ajuste polinomial de Tabelas de Vapor (0 < T < 300ºC)
    return 2500.9 + 1.805 * T - 2.1 * 1e-4 * pow(T, 2) + 3.5 * 1e-7 * pow(T, 3); // [kJ/kg]
}

//-----------------------------------------------------------------------------

double h_super(double T, double P){

    double T_sat = Tsat(P);
    double h_vsat = Hvsat(T_sat);

    double a = 1.872;
    double b = 0.00055;
    double c = -1.6e-7;

    double integral =
        a*(T - T_sat)
        + (b/2.0)*(pow(T,2) - pow(T_sat,2))
        + (c/3.0)*(pow(T,3) - pow(T_sat,3));

    return h_vsat + integral;  // h_ref = entalpia em Tref
}

//-----------------------------------------------------------------------------

double Psat(double T){

    // Equação de Antoine para o Vapor d'água
    double A, B, C;

    // Escolhe faixa automaticamente
    if(T <= 100.0){
        A = 8.07131;
        B = 1730.63;
        C = 233.426;
    } else {
        A = 8.14019;
        B = 1810.94;
        C = 244.485;
    }

    double P_mmHg = pow(10.0, A - B/(T + C));

    return 0.133322 * P_mmHg; // [kPa]
}

//-----------------------------------------------------------------------------

double Tsat(double P){

    // Equação de Antoine para o Vapor d'água
    double A, B, C;

    // Converter para mmHg
    double P_mmHg = P / 0.133322;

    // Escolhe faixa baseado na pressão (~100°C ≈ 101.325 kPa)
    if(P <= 101.325){
        A = 8.07131;
        B = 1730.63;
        C = 233.426;
    } else {
        A = 8.14019;
        B = 1810.94;
        C = 244.485;
    }

    return B / (A - log10(P_mmHg)) - C; // [ºC]
}

//-----------------------------------------------------------------------------

double calc_T(double P, double x){

    double A = 3.62321*pow(x, 3) + 1.13082*x*x - 0.71947*x + 6.77113;

    double B = -2711.78458*pow(x, 3) - 11.18678*x*x + 47.44337*x - 1395.64615;

    double C = 190.80153;

    double T = - C + B / ( log10(P) - A );

    return T; // [ºC]

    //return Tsat(P); //Caso queira desprezar a EPE

}

//-----------------------------------------------------------------------------

void printResults(StateVec *sv){

    printf("\n\nS = %f kg/h", par->S);
    printf("\nTS = %f kg/h", par->TS);

    for(int i=0; i<par->n; i++) printf("\n\nV[%d] = %f kg/h \nL[%d] = %f kg/h", i+1, sv->V[i], i+1, sv->L[i]);

    printf("\n");

    for(int i=0; i<par->n; i++) printf("\nA[%d] = %f m^2", i+1, sv->A[i]);

    printf("\n");

    for(int i=0; i<par->n; i++) printf("\nT[%d] = %f *C", i+1, sv->T[i]);

    printf("\n");

    for(int i=0; i<par->n; i++) printf("\nT_sat[%d] = %f *C", i+1, sv->T_sat[i]);

    printf("\n");

    for(int i=0; i<par->n; i++) printf("\nx[%d] = %f ", i+1, sv->x[i]);

    printf("\n");

    for(int i=0; i<par->n; i++) printf("\nP[%d] = %f kPa", i+1, sv->P[i]);

    printf("\n");

    for(int i=0; i<par->n; i++) printf("\nEPE[%d] = %f *C", i+1, sv->EPE[i]);

    printf("\n");

    for(int i=0; i<par->n; i++) printf("\ndT[%d] = %f *C", i+1, sv->dT[i]);

    printf("\n");

    printf("\nIt_Area = %d", par->ItTotal);
    printf("\nIt_Sistema = %d", par->ItSistema);

    printf("\n");

}

//-----------------------------------------------------------------------------

StateVec *sv_alloc() {

  StateVec *sv = calloc(1, sizeof(StateVec));

  sv->V     = (double *) calloc( par->n, sizeof(double));
  sv->L     = (double *) calloc( par->n, sizeof(double));
  sv->T     = (double *) calloc( par->n, sizeof(double));
  sv->T_sat = (double *) calloc( par->n, sizeof(double));
  sv->dT    = (double *) calloc( par->n, sizeof(double));
  sv->x     = (double *) calloc( par->n, sizeof(double));
  sv->U     = (double *) calloc( par->n, sizeof(double));
  sv->A     = (double *) calloc( par->n, sizeof(double));
  sv->EPE   = (double *) calloc( par->n, sizeof(double));
  sv->P     = (double *) calloc( par->n, sizeof(double));

  return sv;

}

//-----------------------------------------------------------------------------

void freeVec(StateVec *sv) {

  free(sv->V);
  free(sv->L);
  free(sv->T);
  free(sv->T_sat);
  free(sv->dT);
  free(sv->x);
  free(sv->U);
  free(sv->A);
  free(sv->EPE);
  free(sv->P);
  free(sv);

}

//-----------------------------------------------------------------------------

void AvaliarEstabilidade(StateVec *sv){


    if(par->DT <= 0){
        printf("\nSistema fisicamente inviavel: Delta T <= 0\n");
        exit(1);
    }
    for(int i=1; i<par->n; i++){
        if( sv->T_sat[i-1] - sv->T_sat[i] <= sv->EPE[i] ){
            printf("\n\nEfeito %d inviavel\n", i+1);
            exit(1);
        }
    }

}

//-----------------------------------------------------------------------------
