// sites for crystals informations
// www.newlightphotonics.com
// www.unitedcrystals.com
// www.lphotonics.com
// www.redoptronics.com
// www.optocity.com


// crystals index
#include <stdbool.h>
#include <math.h>
#include "pdc.h"

#if 1
// note: This have very small effect
#define TEMP_ADJ(n2, k) n2 = pow2(sqrt(n2) + (k)*temp0)
#else
#define TEMP_ADJ(n2, k)
#endif

// KNbO3 Ref 2 specific
static void KNBO3_index(double lambda, double t, vec3 *n);

// return crystal principal refractive indices (squared) for given crystal type and lamda
void get_crystal_index(enum e_crystal_type crystal_type, double lambda_nm, double temp, vec3 *n2, struct crystal_info_t *inf)
{
  double lambda = lambda_nm * 0.001;             // convert to micrometers
  double lambda2 = lambda * lambda;
  double lambda4;                                // note: defined if used
  double temp0 = temp - 20.0;
  switch (crystal_type)
  {
    default:
      PDC_error("get_crystal_index: Invalid enum"); // invalid enum, define BBO as default
    case (ct_BBO_1):
      if (inf)
      {
        // https://www.newlightphotonics.com/Nonlinear-Optical-Crystals/BBO-Crystals
        // http://www.lphotonics.com/products/Non-Linear%20Crystals/BBO.htm
        inf->name = "BBO (Beta-BaB2O4) Ref 1";   // Beta-Barium Borate
        inf->uniaxial = true;
        inf->temp_en = true;                     // set to true if temp correction datas defined
        inf->min_lambda_nm = 189;                // Transparency range nm
        inf->max_lambda_nm = 3500;
        return;
      }
      n2->x = 2.7359 + 0.01878/(lambda2 - 0.01822) - 0.01354*lambda2;
      n2->z = 2.3753 + 0.01224/(lambda2 - 0.01667) - 0.01516*lambda2;
      if (temp0 != 0.0)
      {
        TEMP_ADJ(n2->x,  -9.3e-6);
        TEMP_ADJ(n2->z, -16.6e-6);
      }
      n2->y = n2->x;
    break;
    case (ct_BBO_2):
      if (inf)
      {
        // Graham, A. Zalkin, J. Appl. Phys., 62 (1987)
        inf->name = "BBO Ref 2";
        inf->uniaxial = true;
        inf->temp_en = true;
        inf->min_lambda_nm = 200;
        inf->max_lambda_nm = 2800;
        return;
      }
      n2->x = 2.7405 + 0.0184/(lambda2 - 0.0179) - 0.0155*lambda2;
      n2->z = 2.3730 + 0.0128/(lambda2 - 0.0156) - 0.0044*lambda2;
      if (temp0 != 0.0)
      {
        TEMP_ADJ(n2->x,  -9.3e-6);
        TEMP_ADJ(n2->z, -16.6e-6);
      }
      n2->y = n2->x;
    break;
    case (ct_BIBO_1):
      if (inf)
      {
        // https://www.newlightphotonics.com/Nonlinear-Optical-Crystals/BiBO-Crystals
        // https://www.newlightphotonics.com/SPDC-Components
        // http://www.lphotonics.com/products/Non-Linear%20Crystals/BIBO.htm
        // http://www.redoptronics.com/BIBO-crystal.html
        inf->name = "BIBO (BiB3O6) Ref 1";
        inf->uniaxial = false;
        inf->temp_en = false;
        inf->min_lambda_nm = 286;
        inf->max_lambda_nm = 2500;
        return;
      }
      n2->x = 3.0740 + 0.0323/(lambda2-0.0316) - 0.01337*lambda2;
      n2->y = 3.1685 + 0.0373/(lambda2-0.0346) - 0.01750*lambda2;
      n2->z = 3.6545 + 0.0511/(lambda2-0.0371) - 0.0226*lambda2;
#if 0
      // wrong coefficients, are Thermal Expansion
      TEMP_ADJ(n2->x,   4.8e-5);
      TEMP_ADJ(n2->y,   4.4e-6);
      TEMP_ADJ(n2->z, -2.69e-5);
#endif
    break;
    case (ct_BIBO_2):
      if (inf)
      {
        inf->name = "BIBO Ref 2";
        inf->uniaxial = false;
        inf->temp_en = false;
        inf->min_lambda_nm = 286;
        inf->max_lambda_nm = 2500;
        return;
      }
      n2->x = 3.0740 + 0.0323/(lambda2-0.0316) - 0.01337*lambda2;
      n2->y = 3.1685 + 0.0373/(lambda2-0.0346) - 0.01750*lambda2;
      n2->z = 3.6545 + 0.0511/(lambda2-0.0371) - 0.0226*lambda2;
#if 0
      // wrong coefficients, are Thermal Expansion
      TEMP_ADJ(n2->x,   4.8e-5);
      TEMP_ADJ(n2->y,   4.4e-6);
      TEMP_ADJ(n2->z, -2.69e-5);
#endif
    break;
    case (ct_KTA_1):
      if (inf)
      {
        // https://www.newlightphotonics.com/Nonlinear-Optical-Crystals/KTA-Crystals
        // http://www.lphotonics.com/products/Non-Linear%20Crystals/KTA.htm
        inf->name = "KTA (KTiAsO4) Ref 1";
        inf->uniaxial = false;
        inf->temp_en = true;
        inf->min_lambda_nm = 350;
        inf->max_lambda_nm = 5300;
        return;
      }

#if 0
      // newlightphotonics
      // produce < 0 results at 450nm ?
      n2->x = 1.90713 +1.23522*lambda2 / (lambda2-0.19692) - 0.01025*lambda2;
      n2->y = 2.15912 +1.00099*lambda2 / (lambda2-0.21844) - 0.01096*lambda2;
      n2->z = 2.14768 +1.29559*lambda2 / (lambda2-0.22719) - 0.01436*lambda2;
#else
      // http://www.lphotonics.com/products/Non-Linear%20Crystals/KTA.htm
      #define SELLMEIER_KTA(Ai,Bi,Ci,Di) Ai+(Bi*lambda2)/(lambda2-pow2(Ci))-Di*lambda2
      n2->x = SELLMEIER_KTA(1.90713, 1.23522, 0.19692, 0.01025);
      n2->y = SELLMEIER_KTA(2.15912, 1.00099, 0.21844, 0.01096);
      n2->z = SELLMEIER_KTA(2.14768, 1.29559, 0.22719, 0.01436);
#endif
      if (temp0 != 0.0)
      {
        TEMP_ADJ(n2->x, 1.1e-5);
        TEMP_ADJ(n2->y, 1.3e-5);
        TEMP_ADJ(n2->z, 1.6e-5);
      }
    break;
    case (ct_KTA_2):
      if (inf)
      {
        // D. L. Fenimore, K. L. Schepler, U. B. Ramabadran,
        // S. R. McPherson, "Infrared corrected Sellmeier coefficients
        // for potassium titaNyl arsenate," J. Opt. Soc. B, 12 (5),
        // 794-796 (May 1995)
        inf->name = "KTA Ref 2";
        inf->uniaxial = false;
        inf->temp_en = false;
        inf->min_lambda_nm = 350;       // note: copy from KTA Ref 1
        inf->max_lambda_nm = 5000;
        return;
      }
      n2->x = 1.90713 + 1.23522/(1 - pow2(0.19692/lambda)) - (0.01025*lambda2);
      n2->y = 2.15912 + 1.00099/(1 - pow2(0.21844/lambda)) - (0.01096*lambda2);
      n2->z = 2.14786 + 1.29559/(1 - pow2(0.22719/lambda)) - (0.01436*lambda2);
#if 0
      TEMP_ADJ(n2->x, 1.1e-5);          // note: copy from KTA Ref 1
      TEMP_ADJ(n2->y, 1.3e-5);
      TEMP_ADJ(n2->z, 1.6e-5);
#endif
    break;
    case (ct_KTP_1):
      if (inf)
      {
        // http://www.redoptronics.com/KTP-crystal.html
        inf->name = "KTP (KTiOPO4) Ref 1";
        inf->uniaxial = false;
        inf->temp_en = true;
        inf->min_lambda_nm = 350;
        inf->max_lambda_nm = 4500;
        return;
      }
      n2->x = 2.10468 + 0.89342*lambda2/(lambda2-0.04438) - 0.01036*lambda2;
      n2->y = 2.14559 + 0.87629*lambda2/(lambda2-0.0485)  - 0.01173*lambda2;
      n2->z = 1.9446  +  1.3617*lambda2/(lambda2-0.047)   - 0.01491*lambda2;
      if (temp0 != 0.0)
      {
        TEMP_ADJ(n2->x, 1.1e-5);
        TEMP_ADJ(n2->y, 1.3e-5);
        TEMP_ADJ(n2->z, 1.6e-5);
      }
    break;
    case (ct_KTP_2):
      if (inf)
      {
        // https://unitedcrystals.com/KTPProp.html
        // https://www.newlightphotonics.com/Nonlinear-Optical-Crystals/KTP-Crystals
        // http://www.lphotonics.com/products/Non-Linear%20Crystals/KTP.htm
        inf->name = "KTP ref 2";
        inf->uniaxial = false;
        inf->temp_en = true;
        inf->min_lambda_nm = 350;
        inf->max_lambda_nm = 3500;
        return;
      }
      n2->x = 3.0065 + 0.03901 / (lambda2-0.04251) - 0.01327*lambda2;
      n2->y = 3.0333 + 0.04154 / (lambda2-0.04547) - 0.01408*lambda2;
      // n2->z = 3.0065 + 0.05694 / (lambda2-0.05658) - 0.01682*lambda2;  // from newlightphotonics, seem incorrect
      n2->z = 3.3134 + 0.05694 / (lambda2 - 0.05658) - 0.01682*lambda2;   // fromm unitedcrystals.com, seem ok

      if (temp0 != 0.0)
      {
        TEMP_ADJ(n2->x, 1.1e-5);         // ok Thermo-optic Coefficients
        TEMP_ADJ(n2->y, 1.3e-5);
        TEMP_ADJ(n2->z, 1.6e-5);
      }
    break;
    case (ct_KTP_3):
      if (inf)
      {
        // H. Vanherzeele, J. D. Bierlein, F. C. Zumsteg, Appl. Opt., 27,
        // 3314 (1988)
        inf->name = "KTP ref 3";
        inf->uniaxial = false;
        inf->temp_en = true;
        inf->min_lambda_nm = 350;        // copy from ktp 1
        inf->max_lambda_nm = 3500;
        return;
      }
      n2->x = 2.1146 + 0.89188/(1 - pow2(0.20861/lambda)) - (0.01320*lambda2);
      n2->y = 2.1518 + 0.87862/(1 - pow2(0.21801/lambda)) - (0.01327*lambda2);
      n2->z = 2.3136 + 1.00012/(1 - pow2(0.23831/lambda)) - (0.01679*lambda2);
      if (temp0 != 0.0)
      {
        TEMP_ADJ(n2->x, 1.1e-5);
        TEMP_ADJ(n2->y, 1.3e-5);
        TEMP_ADJ(n2->z, 1.6e-5);
      }
    break;
    case (ct_KDP_1):
      if (inf)
      {
        // https://www.newlightphotonics.com/Nonlinear-Optical-Crystals/KDP-DKDP-Crystals
        // http://www.lphotonics.com/products/Non-Linear%20Crystals/KDP.htm
        inf->name = "KDP (KH2PO4) Ref 1";
        inf->uniaxial = true;
        inf->temp_en = false;
        inf->min_lambda_nm = 200;
        inf->max_lambda_nm = 1500;
        return;
      }
      n2->x = 2.259276 + 13.005522*lambda2 / (lambda2 - 400) + 0.01008956  / (lambda2 - 0.012942625);   // no^2
      n2->z = 2.132668 + 3.2279924*lambda2 / (lambda2 - 400) + 0.008637494 / (lambda2 - 0.012281043);   // ne^2
#if 0
      TEMP_ADJ(n2->x, 0);              // todo: find values
      TEMP_ADJ(n2->z, 0);
#endif
      n2->y = n2->x;
    break;
    case (ct_KDP_2):
      if (inf)
      {
        // Zernike, Frits, Jr., J. Opt. Soc. Am., 54, 1215 (1964)
        inf->name = "KDP Ref 2";
        inf->uniaxial = true;
        inf->temp_en = false;
        inf->min_lambda_nm = 200;
        inf->max_lambda_nm = 1500;
        return;
      }
      n2->x = 0.01011279/(lambda2 - 0.01294238) +	0.03249268/(0.0025 - 1/lambda2) + 2.260476;
      n2->z = 0.00865325/(lambda2 - 0.01229326) +	0.00806984/(0.0025 - 1/lambda2) + 2.133831;
#if 0
      TEMP_ADJ(n2->x, 0);              // todo: find values
      TEMP_ADJ(n2->z, 0);
#endif
      n2->y = n2->x;
    break;
    case (ct_KDP_3):
      if (inf)
      {
        // Barnes, n.P., D.J. Gettemy, R.S. Adhav, J. Opt. Soc. Am.,
        inf->name = "KDP Ref 3";
        inf->uniaxial = true;
        inf->temp_en = false;
        inf->min_lambda_nm = 200;  // 410  note: extended to allow pm
        inf->max_lambda_nm = 1500; // 650
        return;
      }
      n2->x = 1.0 + 1.24361*lambda2/(lambda2 - 0.00959);
      n2->z = 1.0 + 1.12854*lambda2/(lambda2 - 0.00841);
#if 0
      TEMP_ADJ(n2->x, 0);              // todo: find values
      TEMP_ADJ(n2->z, 0);
#endif
      n2->y = n2->x;
    break;
    case (ct_DKDP_1):
      if (inf)
      {
        // https://www.newlightphotonics.com/Nonlinear-Optical-Crystals/KDP-DKDP-Crystals
        inf->name = "DKDP/KD*P (KD2PO4) Ref 1";  // Potassium Dideuterium Phosphate
        inf->uniaxial = true;
        inf->temp_en = false;
        inf->min_lambda_nm = 200;
        inf->max_lambda_nm = 1600;
        return;
      }
      n2->x = 2.240921 + 2.246956*lambda2/(lambda2 - 126.92073) + 0.009676/(lambda2 - 0.0156203);  // no^2
      n2->z = 2.126019 + 0.784404*lambda2/(lambda2 - 123.40344) + 0.008578/(lambda2 - 0.0119913);  // ne^2
#if 0
      TEMP_ADJ(n2->x, 0);              // todo: find values
      TEMP_ADJ(n2->z, 0);
#endif
      n2->y = n2->x;
    break;
    case (ct_DKDP_2):
      if (inf)
      {
        // http://www.lphotonics.com/products/Non-Linear%20Crystals/KDP.htm
        inf->name = "DKDP/KD*P Ref 2";  // Potassium Dideuterium Phosphate
        inf->uniaxial = true;
        inf->temp_en = false;
        inf->min_lambda_nm = 200;
        inf->max_lambda_nm = 1600;
        return;
      }
      n2->x = 1.9575544+0.2901391*lambda2/(lambda2-0.0281399)-0.02824391*lambda2+0.004977826*lambda2;  // no^2
      n2->z = 1.5005779+0.6276034*lambda2/(lambda2-0.0131558)-0.01054063*lambda2+0.002243821*lambda2;  // ne^2
#if 0
      TEMP_ADJ(n2->x, 0);              // todo: find values
      TEMP_ADJ(n2->z, 0);
#endif
      n2->y = n2->x;
    break;
    case (ct_LBO_1):
      if (inf)
      {
        // https://www.newlightphotonics.com/Nonlinear-Optical-Crystals/LBO-Crystals
        inf->name = "LBO (LiB3O5) Ref 1";
        inf->uniaxial = false;
        inf->temp_en = true;
        inf->min_lambda_nm = 160;
        inf->max_lambda_nm = 2600;
        return;
      }
      lambda4 = lambda2*lambda2;
      n2->x = 2.454140 + 0.011249 / (lambda2-0.011350) - 0.014591*lambda2 - 6.60e-5*lambda4;
      n2->y = 2.539070 + 0.012711 / (lambda2-0.012523) - 0.018540*lambda2 + 2.00e-4*lambda4;
      n2->z = 2.586179 + 0.013099 / (lambda2-0.011893) - 0.017968*lambda2 - 2.26e-4*lambda4;
      if (temp0 != 0.0)
      {
#if 1
        vec3 dn_dt;                  // dn/dT, lambda in um
        dn_dt.x = (-3.76*lambda +  2.30)*1e-6;
        dn_dt.y = ( 6.01*lambda - 19.40)*1e-6;
        dn_dt.z = ( 1.50*lambda -  9.70)*1e-6;
        TEMP_ADJ(n2->x, dn_dt.x);
        TEMP_ADJ(n2->y, dn_dt.y);
        TEMP_ADJ(n2->z, dn_dt.z);
#else
        // https://www.crylink.com/wp-content/uploads/PDF/LBO-Nonlinear-Crystal-Dadasheet-Laser-Crylink.pdf
        TEMP_ADJ(n2->x, -9.3e-6);
        TEMP_ADJ(n2->y, -13.6e-6);
        TEMP_ADJ(n2->z, (-6.3 - 2.1*lambda)*1e-6);
#endif
      }
    break;
    case (ct_LBO_2):
      if (inf)
      {
        // Velsko et al, "Phase-matched harmonic generation in Lithum Triborate (LBO)"
        // IEEE J. Quant. Eletron. 27, 2182-2192(1991)
        // 0.365 - > 1.1 microns
        inf->name = "LBO Ref 2";
        inf->uniaxial = false;
        inf->temp_en = false;
        inf->min_lambda_nm = 160;
        inf->max_lambda_nm = 2600;
        return;
      }
      n2->x = 2.4543 + 0.011413/(lambda2-0.0094981) - 0.0138900*lambda2;
      n2->y = 2.5382 + 0.012830/(lambda2-0.01387)   - 0.017034*lambda2;
      n2->z = 2.5854 + 0.013065/(lambda2-0.011617)  - 0.018146*lambda2;
#if 0
      TEMP_ADJ(n2->x, 0);              // todo: define
      TEMP_ADJ(n2->y, 0);
      TEMP_ADJ(n2->z, 0);
#endif
    break;
    case (ct_KNbO3_1):
      if (inf)
      {
        // http://www.lphotonics.com/products/Non-Linear%20Crystals/NLO%20Crystal%20KNbO3.htm
        inf->name = "KNbO3 Ref 1";
        inf->uniaxial = false;
        inf->temp_en = false;
        inf->min_lambda_nm = 400;
        inf->max_lambda_nm = 4500;
        return;
      }
      n2->x = 4.4208 + 0.10044/(lambda2 - 0.054084) - 0.019592*lambda2;
      n2->y = 4.8355 + 0.12839/(lambda2 - 0.056342) - 0.025379*lambda2;
      n2->z = 4.9873 + 0.15149/(lambda2 - 0.064143) - 0.028775*lambda2;
#if 0
      TEMP_ADJ(n2->x, 0);
      TEMP_ADJ(n2->y, 0);
      TEMP_ADJ(n2->z, 0);
#endif
    break;
    case (ct_KNbO3_2):
      {
        vec3 n;
        if (inf)
        {
          inf->name = "KNbO3 Ref 2";
          inf->uniaxial = false;
          inf->temp_en = true;
          inf->min_lambda_nm = 400;
          inf->max_lambda_nm = 4500;
          return;
        }
        KNBO3_index(lambda, temp, &n);
        n2->x = n.x*n.x;
        n2->y = n.y*n.y;
        n2->z = n.z*n.z;
      }
    break;
    case (ct_LiNbO3):
      if (inf)
      {
        // https://www.newlightphotonics.com/Nonlinear-Optical-Crystals/LN-MGOLN-Crystals
        inf->name = "LiNbO3 Lithium Niobate (pure LN)";
        inf->uniaxial = true;
        inf->temp_en = true;
        inf->min_lambda_nm = 420;
        inf->max_lambda_nm = 5200;
        return;
      }
      n2->x = 4.9048 + 0.11768  / (lambda2 - 0.04750) - 0.027169*lambda2;   // no^2
      n2->z = 4.5820 + 0.099169 / (lambda2 - 0.04443) - 0.021950*lambda2;   // ne^2
      if (temp0 != 0.0)
      {
        // note: at 1.4 um
        TEMP_ADJ(n2->x, -0.874e-6);
        TEMP_ADJ(n2->z, 39.073e-6);
      }
      n2->y = n2->x;
    break;
    case (ct_AgGaS2):
      if (inf)
      {
        // https://www.newlightphotonics.com/Nonlinear-Optical-Crystals/AgGaS2-Crystals
        inf->name = "AgGaS2 Silver Gallium Sulfide";
        inf->uniaxial = true;
        inf->temp_en = true;
        inf->min_lambda_nm = 500;
        inf->max_lambda_nm = 13200;
        return;
      }
      n2->x = 3.3970 + 2.3982 / (1-0.09311/lambda2) + 2.1640 / (1 -    950 / lambda2);   // no^2
      n2->z = 3.5873 + 1.9533 / (1-0.11066/lambda2) + 2.3391 / (1 - 1030.7 / lambda2);   // ne^2
      if (temp0 != 0.0)
      {
        TEMP_ADJ(n2->x, 15.4e-5);
        TEMP_ADJ(n2->z, 15.5e-5);
      }
      n2->y = n2->x;
    break;
    case (ct_RTP_1):
      if (inf)
      {
        // http://www.lphotonics.com/products/Non-Linear%20Crystals/RTP.htm
        // https://www.newlightphotonics.com/E-O-Crystals/RTP-for-EO
        inf->name = "RTP (RbTiOPO4) Rubidium Titanyl Phosphate";
        inf->uniaxial = false;
        inf->temp_en = false;
        inf->min_lambda_nm = 350;
        inf->max_lambda_nm = 4500;
        return;
      }

#if 1
      // http://www.lphotonics.com/products/Non-Linear%20Crystals/RTP.htm
      #define SELLMEIER_RTP(Ai,Bi,Ci,Di) Ai+Bi*(1.0-pow2(Ci/lambda))-Di*lambda2
      n2->x = SELLMEIER_RTP(2.15559, 0.93307, 0.20994, 0.01452);
      n2->y = SELLMEIER_RTP(2.38494, 0.73603, 0.23891, 0.01583);
      n2->z = SELLMEIER_RTP(2.27723, 1.11030, 0.23454, 0.01995);
#endif
#if 0
      // http://www.optocity.com/RTP.htm (same as lphotonics)
      n2->x=2.15559 + 0.93307*(1 - pow2(0.20994/lambda)) - 0.01452*lambda2;
      n2->y=2.38494 + 0.73603*(1 - pow2(0.23891/lambda)) - 0.01583*lambda2;
      n2->z=2.27723 + 1.11030*(1 - pow2(0.23454/lambda)) - 0.01995*lambda2;
#endif
#if 0
      // https://www.newlightphotonics.com/E-O-Crystals/RTP-for-EO
      // ERROR, produce < indices
      n2->x = 2.1556  + ( 0.93307*lambda2)/(lambda2-0.20994)-0.01452*lambda2;
      n2->y = 2.3849  + (0.073603*lambda2)/(lambda2-0.23891)-0.01583*lambda2;
      n2->z = 2.27723 + ( 1.11030*lambda2)/(lambda2-0.23454)-0.01995*lambda2;
#endif

#if 0
      // dn/dT -0.029 nm / °C
      TEMP_ADJ(n2->x, 0);
      TEMP_ADJ(n2->y, 0);
      TEMP_ADJ(n2->z, 0);
#endif
    break;
    case (ct_YVO4_1):
      if (inf)
      {
        // http://www.redoptronics.com/YVO4-crystal.html
        // https://www.newlightphotonics.com/Laser-Crystals/Nd-doped-YVO4-Crystals
        // https://unitedcrystals.com/YVO4Prop.html
        inf->name = "YVO4 Yttrium orthovanadate";
        inf->uniaxial = true;
        inf->temp_en = true;
        inf->min_lambda_nm = 400;
        inf->max_lambda_nm = 5000;
        return;
      }
      n2->x = 3.77834 + 0.069736 / (lambda2 - 0.04724) - 0.0108133*lambda2;   // no^2
      n2->z = 4.59905 + 0.110534 / (lambda2 - 0.04813) - 0.0122676*lambda2;   // ne^2
      // (error/redoptronics) n2->z = 24.5905 + 0.110534 / (lambda2 - 0.04813) - 0.0122676*lambda2;   // ne^2
      if (temp0 != 0.0)
      {
        // dna/dT=8.5e-6
        // dnb/dT=8.5e-6
        // dnc/dT=3.0e-6
        TEMP_ADJ(n2->x, 8.5e-6);
        TEMP_ADJ(n2->z, 3.0e-6);
      }
      n2->y = n2->x;
    break;
  }
}


// ----------------------------------------------
// code for ct_KNbO3_2 only
// ----------------------------------------------

static double Sellmeir(double l, double l1, double l2, double s1, double s2, double d)
{
  double arg, res;
  arg = 1 + s1 * l*l * l1*l1 / (l*l - l1*l1) 
          + s2 * l*l * l2*l2 / (l*l - l2*l2)
          - d * l*l;
#if 0
  res = sqrt(arg);
#else
  res = (arg > 1) ? sqrt(arg) : 4;         // avoid FP error if out of range (n(lambda) graph)
#endif
  return res;
}

// special for KNBO3, index for temperature
static void KNBO3_index_T(double Lambda, int T, vec3 *n)
{
  // Local Variables
  double Nx, Ny, Nz, D1, L1, L2, S1, S2;

  if (T == 22)
  {
    D1 = 1.943289e-2;
    L1 = 2.55227711e-01;
    L2 = 1.19714173e-01;
    S1 = 1.609170e+01;
    S2 = 1.654431e+02;
    Nx = Sellmeir(Lambda,L1,L2,S1,S2,D1);

    D1 = 2.517432e-2;
    L1 = 2.58157325e-01;
    L2 = 1.29092115e-01;
    S1 = 2.005519e+01;
    S2 = 1.498408e+02;
    Ny = Sellmeir(Lambda,L1,L2,S1,S2,D1);
              
    D1 = 2.845018e-2;
    L1 = 2.72745560e-01;
    L2 = 1.37003854e-01;
    S1 = 1.937347e+01;
    S2 = 1.354992e+02;
    Nz = Sellmeir(Lambda,L1,L2,S1,S2,D1);
  }
  else
  if (T == 50)
  {
    D1 = 1.945428e-2;
    L1 = 2.52752522e-01;
    L2 = 1.11742242e-01;
    S1 = 1.824447e+01;
    S2 = 1.811303e+02;
    Nx = Sellmeir(Lambda,L1,L2,S1,S2,D1);

    D1 = 2.514857e-2;
    L1 = 2.58664133e-01;
    L2 = 1.27592173e-01;
    S1 = 2.027475e+01;
    S2 = 1.523380e+02;
    Ny = Sellmeir(Lambda,L1,L2,S1,S2,D1);

    D1 = 2.827127e-2;
    L1 = 2.71398770e-01;
    L2 = 1.30593386e-01;
    S1 = 2.066250e+01;
    S2 = 1.441607e+02;
    Nz = Sellmeir(Lambda,L1,L2,S1,S2,D1);
  }
  else 
  if (T == 75)
  {
    D1 = 1.952880e-2;
    L1 = 2.54675819e-01;
    L2 = 1.13972981e-01;
    S1 = 1.756954e+01;
    S2 = 1.766712e+02;
    Nx = Sellmeir(Lambda,L1,L2,S1,S2,D1);

    D1 = 2.514893e-2;
    L1 = 2.60772236e-01;
    L2 = 1.29504861e-01;
    S1 = 1.933691e+01;
    S2 = 1.504971e+02;
    Ny = Sellmeir(Lambda,L1,L2,S1,S2,D1);

    D1 = 2.807510e-2;
    L1 = 2.72530570e-01;
    L2 = 1.31949811e-01;
    S1 = 2.006891e+01;
    S2 = 1.427868e+02;
    Nz = Sellmeir(Lambda,L1,L2,S1,S2,D1);
  }
  else
  if (T == 100)
  {
    D1 = 1.968455e-2;
    L1 = 2.62219697e-01;
    L2 = 1.25100015e-01;
    S1 = 1.401940e+01;
    S2 = 1.584156e+02;
    Nx = Sellmeir(Lambda,L1,L2,S1,S2,D1);

    D1 = 2.515860e-2;
    L1 = 2.65628695e-01;
    L2 = 1.37103071e-01;
    S1 = 1.660832e+01;
    S2 = 1.420508e+02;
    Ny = Sellmeir(Lambda,L1,L2,S1,S2,D1);

    D1 = 2.796330e-2;
    L1 = 2.81955899e-01;
    L2 = 1.51431159e-01;
    S1 = 1.437713e+01;
    S2 = 1.233481e+02;
    Nz = Sellmeir(Lambda,L1,L2,S1,S2,D1);
  }
  else
  if (T == 140)
  {
    D1 = 1.982648e-2;
    L1 = 2.58712656e-01;
    L2 = 1.18491161e-01;
    S1 = 1.648284e+01;
    S2 = 1.677579e+02;
    Nx = Sellmeir(Lambda,L1,L2,S1,S2,D1);

    D1 = 2.508202e-2;
    L1 = 2.67779444e-01;
    L2 = 1.38395441e-01;
    S1 = 1.604692e+01;
    S2 = 1.407942e+02;
    Ny = Sellmeir(Lambda,L1,L2,S1,S2,D1);

    D1 = 2.771253e-2;
    L1 = 2.74864037e-01;
    L2 = 1.35282050e-01;
    S1 = 1.887445e+01;
    S2 = 1.388675e+02;
    Nz = Sellmeir(Lambda,L1,L2,S1,S2,D1);
  }
  else
  if (T == 180)
  {
    D1 = 2.004523e-2;
    L1 = 2.52773288e-01;
    L2 = 9.88011467e-02;
    S1 = 2.147866e+01;
    S2 = 2.156493e+02;
    Nx = Sellmeir(Lambda,L1,L2,S1,S2,D1);

    D1 = 2.511282e-2;
    L1 = 2.73698130e-01;
    L2 = 1.43185621e-01;
    S1 = 1.389662e+01;
    S2 = 1.372157e+02;
    Ny = Sellmeir(Lambda,L1,L2,S1,S2,D1);

    D1 = 2.762299e-2;
    L1 = 2.71016125e-01;
    L2 = 1.19793058e-01;
    S1 = 2.212020e+01;
    S2 = 1.628852e+02;
    Nz = Sellmeir(Lambda,L1,L2,S1,S2,D1);
  }
  else
  {
    PDC_error("KNBO3_index_T: invalid temperture");
    Nx = 1.0;
    Ny = 1.0;
    Nz = 1.0;
  }

  n->x = Nx;
  n->y = Ny;
  n->z = Nz;
}

// quadratic interpolation / extrapolation
static void quad_int(double t, double t1, double t2, double t3, vec3 *q)
{
  q->x = ((t-t2)*(t-t3))/((t1-t2)*(t1-t3));
  q->y = ((t-t1)*(t-t3))/((t2-t1)*(t2-t3));
  q->z = ((t-t1)*(t-t2))/((t3-t1)*(t3-t2));
}

static void KNBO3_index(double lambda, double t, vec3 *n)
{
  //  J. Opt. Soc. Am. B/Vol.9,No.3/March 1992
  // 	Refractive indices of orthorhombic KNbO3
  //	B. Zysset
  vec3 n1, n2, n3, q;
  int t1, t2, t3;

  if      (t < 75)  {	t1 =  22; t2 =  50; t3 =  75; }
  else if (t < 100) { t1 =  50; t2 =  75; t3 = 100; }
  else if (t < 140) { t1 =  75; t2 = 100; t3 = 140; }
  else              { t1 = 100; t2 = 140; t3 = 180; }

  KNBO3_index_T(lambda, t1, &n1);
  KNBO3_index_T(lambda, t2, &n2);
  KNBO3_index_T(lambda, t3, &n3);

  // Quadratic Interpolation / Extrapolation
  quad_int(t, t1, t2, t3, &q);

  n->x = n1.x*q.x + n2.x*q.y + n3.x*q.z;
  n->y = n1.y*q.x + n2.y*q.y + n3.y*q.z;
  n->z = n1.z*q.x + n2.z*q.y + n3.z*q.z;
}