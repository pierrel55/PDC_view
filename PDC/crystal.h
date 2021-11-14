// defined crystals
enum e_crystal_type
{
  ct_BBO_1 = 0,
  ct_BBO_2,
  ct_BIBO_1,
  ct_BIBO_2,
  ct_KTA_1,
  ct_KTA_2,
  ct_KTP_1,
  ct_KTP_2,
  ct_KTP_3,
  ct_KDP_1,
  ct_KDP_2,
  ct_KDP_3,
  ct_DKDP_1,
  ct_DKDP_2,
  ct_LBO_1,
  ct_LBO_2,
  ct_KNbO3_1,
  ct_KNbO3_2,
  ct_LiNbO3,
  ct_AgGaS2,
  ct_RTP_1,
  ct_YVO4_1,
  ct_list_size,                                  // not crystal, list size, must be last element
};

// information about crystal
struct crystal_info_t
{
  const char *name;
  bool uniaxial;                                 // ny = nx
  bool temp_en;                                  // temperature correction can be applyed (coeff defined in code)
  double min_lambda_nm;
  double max_lambda_nm;
};

// return crystal principal refractive indices for given crystal type and lamda or crystal informations.
void get_crystal_index(enum e_crystal_type, double lambda_nm, double temp, vec3 *n2, struct crystal_info_t *inf);
