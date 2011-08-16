/* preparation of Touschek */
/* defined constants for word lengths etc. */
#define ALIGN_MAX 14        /* alignment error array length */
#define EFIELD_TAB 42       /* field error array length for ESAVE table */
#define FIELD_MAX 42        /* field error array length */
#define SEQ_DUMP_LEVEL 0    /* chooses amount of dumped output */
#define NAME_L 48           /* internal name length */
#define TITLE_SIZE 114      /* Size of the title for gnuplot ploting in tracking mode (ETDA 24/06/2004) */
#define PTC_NAMES_L 13      /* Number of ptc variables treated in select_ptc_normal (ETDA 10/11/2004)(FRS 06/12/2005) (FRS/VK 20/04/2006) */
#define MAX_ROWS 101        /* Initial size of ptc_normal table */
#define FNAME_L 240         /* for file names */
#define FREECODE 380226     /* check-code to avoid multiple "free" */
#ifdef _MEM_LEAKS
#define MTABLE_SIZE 1000000
int item_no=-1;
int* mtable[MTABLE_SIZE];
#endif
#define AUX_LG 50000        /* initial size for ancillary buffers */
#define INVALID 1.e20       /* used for erroneous value requests */
#define MAX_ITEM  1000      /* initial # of items in tok_list etc. */
#define MAX_D_ITEM 30000    /* initial storage size for doubles */
#define MAX_LINE 20000      /* max. input line length (has to stay fixed) */
#define MAX_LOOP 100        /* max. count for (possibly circular) calls */
#define MAX_COND 100        /* max. nesting level for "if" and "while" */
#define MAX_TYPE 11         /* for SXF output */
#define MAX_TAG 50          /* for SXF output */
#define CHAR_BUFF_SIZE 100000 /* size of dynamic char_buff members */
#define IN_BUFF_SIZE 500000 /* initial size of buffer for command groups */
#define LINE_FILL 240        /* max. line length -2 for "save" output */
#define LINE_F_MAD8 70      /* the same, for mad-8 format */
#define MADX_LINE_MAX 78         /* for SXF output */
#define MAX_RAND 1000000000 /* for random generator */
#define NR_RAND 55          /* for random generator */
#define NJ_RAND 24          /* for random generator */
#define ND_RAND 21          /* for random generator */
#define MATCH_WORK 10       /* no. of work spaces in matching */
#define USER_TABLE_LENGTH 100 /* initial length of user defined tables */
#define MAXARRAY 1000       /* max. length of apex tables in aperture module*/
#define DQ_DELTAP 1.e-6     /* deltap for difference calculation of chrom. */
/* IA */
#define E_D_MAX 500         /* max. length of extra displacement tables (per element) */
#define E_D_LIST_CHUNK 1000  /* chunk to allocate memory for extra displacement tables */


#define MADX_LONG      1
#define MADX_DOUBLE    2
#define MADX_STRING    3

#define MAX_TFS_ROW 2000  /* max. number of rows for SDDS  conversion */
#define MAX_TFS_COL 500   /* max. number of columns for SDDS conversion */

char* const functs[] = {"dummyfunction", "abs", "sqrt", "exp", "log", "log10",
                        "sin", "cos", "tan", "asin", "acos",
                        "atan", "sinh", "cosh", "tanh", "ranf",
                        "gauss", "tgauss", "table", "exist", "floor","ceil","round","frac",
                        ""}; /* keep "" ! */

const char op_string[] = "-+*/^";
char file_string[] = "file"; /* to avoid local in routine alias */
char vrai[] = "true";        /* to avoid local in routine alias */
char faux[] = "false";       /* to avoid local in routine alias */
const int n_match = 17; /* # of match token lists in cmd_match_base */
const int s_match[] = /* position of first token of command below */
{0, 1, 4, 8, 13, 17, 22, 25, 29, 32, 36, 39, 43, 45, 48, 50, 52, 56};

const int t_match[] = /* order in which the commands are matched */
{0, 1, 16, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
const char* cmd_match_base[] =
{ /*  0 */ "@cmd",
  /*  1 */ "@name", ":", "@cmd",
  /*  2 */ "int", "const", "@name", "=",
  /*  3 */ "int", "const", "@name", ":", "=",
  /*  4 */ "real", "const", "@name", "=",
  /*  5 */ "real", "const", "@name", ":", "=",
  /*  6 */ "int", "@name", "=",
  /*  7 */ "int", "@name", ":", "=",
  /*  8 */ "real", "@name", "=",
  /*  9 */ "real", "@name", ":", "=",
  /* 10 */ "const", "@name", "=",
  /* 11 */ "const", "@name", ":", "=",
  /* 12 */ "@name", "=",
  /* 13 */ "@name", ":", "=",
  /* 14 */ "@name", ":",
  /* 15 */ "@name", "@name",
  /* 16 */ "shared", "@name", ":", "@cmd"};

/* aperture types and # of parameters, needed for twiss table */

char* aperture_types[] =
{
  "circle", "ellipse", "rectangle", "lhcscreen",
  "marguerite", "rectellipse", "racetrack",
  " "  /* blank terminates */
};

/*added 4, 3 and "racetrack" here, IW */

int aperture_npar[] =
{
  1, 2, 2, 3,
  2, 4, 3
};

/* table descriptors: type 1 = int, type 2 = double, type 3 = string;
   internally, however, int are stored as double */

int ap_table_types[] =
{
  3, 2, 2, 2, 3,
  2, 2, 2, 2,
  2, 2, 2,
  2, 2, 2, 2, 2, 2, 2,
  2, 2, 2
};

char* ap_table_cols[] =
{
  "name", "n1", "n1x_m", "n1y_m", "apertype",
  "aper_1", "aper_2", "aper_3", "aper_4",
  "rtol", "xtol", "ytol",
  "s", "betx", "bety", "dx", "dy", "x", "y",
  "on_ap", "on_elem", "spec",
  " "  /* blank terminates */
};

int survey_table_types[] =
{
  3, 2, 2, 2, 2,
  2, 2, 2, 2, 2, 2,
  1, 1, 2,
  /*== jln 11.11.2010 dealt with the new property v_pos as for mech_sep */
  2
  /*==*/
};

char* survey_table_cols[] =
{
  "name", "s", "l", "angle", "x",
  "y", "z", "theta", "phi", "psi", "globaltilt",
  "slot_id", "assembly_id", "mech_sep",
  /*== jln 11.11.2010 dealt with the new property v_pos as for mech_sep */
  "v_pos",
  /*==*/
  " "  /* blank terminates */
};


int efield_table_types[] =
{
  3, 2, 2, 2, 2,
  2, 2, 2, 2, 2,
  2, 2, 2, 2, 2,
  2, 2, 2, 2, 2,
  2, 2, 2, 2, 2,
  2, 2, 2, 2, 2,
  2, 2, 2, 2, 2,
  2, 2, 2, 2, 2,
  2, 2, 2, 2, 2,
  2, 2, 2, 2, 2,
  2, 2, 2, 2, 2,
  2, 2
};

char* efield_table_cols[] =
{
  "name",
  "k0l", "k0sl", "k1l", "k1sl",
  "k2l", "k2sl", "k3l", "k3sl", "k4l",
  "k4sl", "k5l", "k5sl", "k6l", "k6sl",
  "k7l", "k7sl", "k8l", "k8sl", "k9l",
  "k9sl", "k10l", "k10sl", "k11l", "k11sl",
  "k12l", "k12sl", "k13l", "k13sl", "k14l",
  "k14sl", "k15l", "k15sl", "k16l", "k16sl",
  "k17l", "k17sl", "k18l", "k18sl", "k19l",
  "k19sl", "k20l", "k20sl",
  "dx", "dy", "ds", "dphi", "dtheta",
  "dpsi", "mrex", "mrey", "mredx", "mredy",
  "arex", "arey", "mscalx", "mscaly",
  " "  /* blank terminates */
};


char* sxf_table_names[] =
{
  "l","angle", "k0","k0s","k1","k1s",
  "e1","e2","k2","k2s","h1",
  "h2","hgap","fint","k3","k3s",
  "lrad","knl","ksl","ks","volt",
  "lag","harmon","betrf","pg",
  "shunt","tfill","eloss","ex","ey",
  "hkick","vkick","xsize","ysize","sigx",
  "sigy","xma","yma","charge",
  " " /* blank terminates */
};

int twiss_opt_end = 33; /* last column filled by twiss module */
int twiss_mult_end = 78; /* last multipole column filled
                            by complete_twiss_table */
int twiss_fill_end = 97; /* last standard column filled
                            by complete_twiss_table */
/*== jln 11.11.2010 increased twiss_fill_end from 96 to 97 to accomodate for v_pos */

/* warning: modify routine complete_twiss_table in case of changes */
int twiss_table_types[] =
{
  3, 3, 2, 2, 2,
  2, 2, 2, 2, 2,
  2, 2, 2, 2, 2,
  2, 2, 2, 2, 2,
  2, 2, 2, 2, 2,
  2, 2, 2, 2, 2,
  2, 2, 2, 2, 2,
  2, 2, 2, 2, 2,
  2, 2, 2, 2, 2,
  2, 2, 2, 2, 2,
  2, 2, 2, 2, 2,
  2, 2, 2, 2, 2,
  2, 2, 2, 2,
  2, 2, 2, 2,
  2, 2, 2, 2,
  2, 2, 2, 2,
  2, 2, 2, 2,
  2, 2, 2, 2, 2,
  2, 2, 2, 2,
  2, 2, 2, 2,
  1, 1, 2, 
  2, /* v_pos */
  2, 3,
  2, 2, 2, 2, 2, 2,
  2, 2, 2, 2, 2, 2,
  2, 2, 2, 2, 2, 2,
  2, 2, 2, 2, 2, 2,
  2, 2, 2, 2, 2, 2,
  2, 2, 2, 2, 2, 2,
  2, 2, 2, 2, 2,
  2, 2, 2,
  2, 2, 2,
  2, 2, 2,
  2, 2, 2,
  2, 2, 2,
  2, 2, 2,
  2, 2, 2,
  2, 2, 2,
  2, 2, 2,
  /* delta_p dependency terms */
  2,2,2, /* beta11p, beta12p, beta13p */
  2,2,2, /* beta21p, beta22p, beta23p  */
  2,2,2, /* beta31p, beta32p, beta33p  */
  2,2,2, /* alfa11p, alfa12p, alfa13p */
  2,2,2, /* alfa21p, alfa22p, alfa23p */
  2,2,2, /* alfa31p, alfa32p, alfa33p */
  2,2,2, /* gama11p, gama12p, gama13p */
  2,2,2, /* gama21p, gama22p, gama23p */
  2,2,2, /* gama31p, gama32p, gama33p */
  /* end of delta_p dependency terms */
  2, 2, 2, 2,
  /* derivatives of dispersion w.r.t. delta_p */
  2, 2, 2, 2,
  2, 2, 2, 2, /* second order derivatives */
  2, 2, 2, 2, /* third order derivatives */
  /* end of dispersion derivatives w.r.t. delta_p */
  2, 2, 2, /* mu1, mu2, mu3 */
  2, 2, 2, 2, 2, 2,
  2, 2, 2, 2, 2, 2,
  2, 2, 2, 2, 2, 2,
  2, 2, 2, 2, 2, 2,
  2, 2, 2, 2, 2, 2,
  2, 2, 2, 2, 2, 2,
  2
};

char* twiss_table_cols[] =
{
  "name", "keyword", "s", "betx", "alfx",
  "mux", "bety", "alfy", "muy", "x",
  "px", "y", "py", "t", "pt",
  "dx", "dpx", "dy", "dpy", "wx",
  "phix", "dmux", "wy", "phiy", "dmuy",
  "ddx", "ddpx", "ddy", "ddpy", "r11",
  "r12", "r21", "r22", "energy", "l",
  "angle", "k0l", "k0sl", "k1l", "k1sl",
  "k2l", "k2sl", "k3l", "k3sl", "k4l",
  "k4sl", "k5l", "k5sl", "k6l", "k6sl",
  "k7l", "k7sl", "k8l", "k8sl", "k9l",
  "k9sl", "k10l", "k10sl", "k11l", "k11sl", 
  "k12l", "k12sl", "k13l", "k13sl", 
  "k14l", "k14sl", "k15l", "k15sl", 
  "k16l", "k16sl", "k17l", "k17sl", 
  "k18l", "k18sl", "k19l", "k19sl", 
  "k20l", "k20sl", "ksi", "hkick",
  "vkick", "tilt", "e1", "e2", "h1",
  "h2", "hgap", "fint", "fintx",
  "volt", "lag", "freq", "harmon",
  "slot_id","assembly_id","mech_sep",
  /*== jln 11.11.2010 dealt with the new property v_pos as for mech_sep */
  "v_pos",
  /*==*/
  "lrad","parent",
  "re11", "re12", "re13", "re14", "re15", "re16",
  "re21", "re22", "re23", "re24", "re25", "re26",
  "re31", "re32", "re33", "re34", "re35", "re36",
  "re41", "re42", "re43", "re44", "re45", "re46",
  "re51", "re52", "re53", "re54", "re55", "re56",
  "re61", "re62", "re63", "re64", "re65", "re66",
  "kmax", "kmin", "calib", "polarity", "alfa",
  "beta11", "beta12", "beta13",
  "beta21", "beta22", "beta23",
  "beta31", "beta32", "beta33",
  "alfa11", "alfa12", "alfa13",
  "alfa21", "alfa22", "alfa23",
  "alfa31", "alfa32", "alfa33",
  "gama11", "gama12", "gama13",
  "gama21", "gama22", "gama23",
  "gama31", "gama32", "gama33",
  /* delta_p dependency: derivatives of the above Twiss parameters */
  "beta11p","beta12p","beta13p",
  "beta21p","beta22p","beta23p",
  "beta31p","beta32p","beta33p",
  "alfa11p", "alfa12p","alfa13p",
  "alfa21p", "alfa22p","alfa23p",
  "alfa31p", "alfa32p","alfa33p",
  "gama11p", "gama12p","gama13p",
  "gama21p", "gama22p","gama23p",
  "gama31p", "gama32p","gama33p",
  /* end of delta_p dependency */
  "disp1", "disp2", "disp3","disp4",
  /* derivatives of dispersion w.r.t. delta_p */
  "disp1p", "disp2p", "disp3p", "disp4p",
  "disp1p2", "disp2p2", "disp3p2", "disp4p2", /* second order derivatives */
  "disp1p3", "disp2p3", "disp3p3", "disp4p3", /* third order derivatives */
  /* end of dispersion derivatives w.r.t. delta_p */
  "mu1", "mu2", "mu3",
  "eign11", "eign12", "eign13", "eign14", "eign15", "eign16",
  "eign21", "eign22", "eign23", "eign24", "eign25", "eign26",
  "eign31", "eign32", "eign33", "eign34", "eign35", "eign36",
  "eign41", "eign42", "eign43", "eign44", "eign45", "eign46",
  "eign51", "eign52", "eign53", "eign54", "eign55", "eign56",
  "eign61", "eign62", "eign63", "eign64", "eign65", "eign66",
  "n1",
  " "  /* blank terminates */
};

int twiss_sector_table_types[] = {
  3, 2,
  2, 2, 2, 2, 2, 2,
  /* 36 elements for the R-matrix */
  2, 2, 2, 2, 2, 2, 
  2, 2, 2, 2, 2, 2, 
  2, 2, 2, 2, 2, 2, 
  2, 2, 2, 2, 2, 2, 
  2, 2, 2, 2, 2, 2, 
  2, 2, 2, 2, 2, 2, 
  /* 216 elements for the T-matrix */
  2, 2, 2, 2, 2, 2, 
  2, 2, 2, 2, 2, 2, 
  2, 2, 2, 2, 2, 2, 
  2, 2, 2, 2, 2, 2, 
  2, 2, 2, 2, 2, 2, 
  2, 2, 2, 2, 2, 2, 
  2, 2, 2, 2, 2, 2, 
  2, 2, 2, 2, 2, 2, 
  2, 2, 2, 2, 2, 2, 
  2, 2, 2, 2, 2, 2, 
  2, 2, 2, 2, 2, 2, 
  2, 2, 2, 2, 2, 2, 
  2, 2, 2, 2, 2, 2, 
  2, 2, 2, 2, 2, 2, 
  2, 2, 2, 2, 2, 2, 
  2, 2, 2, 2, 2, 2, 
  2, 2, 2, 2, 2, 2, 
  2, 2, 2, 2, 2, 2, 
  2, 2, 2, 2, 2, 2, 
  2, 2, 2, 2, 2, 2, 
  2, 2, 2, 2, 2, 2, 
  2, 2, 2, 2, 2, 2, 
  2, 2, 2, 2, 2, 2, 
  2, 2, 2, 2, 2, 2, 
  2, 2, 2, 2, 2, 2, 
  2, 2, 2, 2, 2, 2, 
  2, 2, 2, 2, 2, 2, 
  2, 2, 2, 2, 2, 2, 
  2, 2, 2, 2, 2, 2, 
  2, 2, 2, 2, 2, 2, 
  2, 2, 2, 2, 2, 2, 
  2, 2, 2, 2, 2, 2, 
  2, 2, 2, 2, 2, 2, 
  2, 2, 2, 2, 2, 2, 
  2, 2, 2, 2, 2, 2, 
  2, 2, 2, 2, 2, 2
};

char* twiss_sector_table_cols[] = {
  "name", "pos",
  "k1", "k2", "k3", "k4", "k5", "k6",
  "r11", "r21", "r31", "r41", "r51", "r61",
  "r12", "r22", "r32", "r42", "r52", "r62",
  "r13", "r23", "r33", "r43", "r53", "r63",
  "r14", "r24", "r34", "r44", "r54", "r64",
  "r15", "r25", "r35", "r45", "r55", "r65",
  "r16", "r26", "r36", "r46", "r56", "r66",
  "t111", "t211", "t311", "t411", "t511", "t611",
  "t121", "t221", "t321", "t421", "t521", "t621",
  "t131", "t231", "t331", "t431", "t531", "t631",
  "t141", "t241", "t341", "t441", "t541", "t641",
  "t151", "t251", "t351", "t451", "t551", "t651",
  "t161", "t261", "t361", "t461", "t561", "t661",
  "t112", "t212", "t312", "t412", "t512", "t612",
  "t122", "t222", "t322", "t422", "t522", "t622",
  "t132", "t232", "t332", "t432", "t532", "t632",
  "t142", "t242", "t342", "t442", "t542", "t642",
  "t152", "t252", "t352", "t452", "t552", "t652",
  "t162", "t262", "t362", "t462", "t562", "t662",
  "t113", "t213", "t313", "t413", "t513", "t613",
  "t123", "t223", "t323", "t423", "t523", "t623",
  "t133", "t233", "t333", "t433", "t533", "t633",
  "t143", "t243", "t343", "t443", "t543", "t643",
  "t153", "t253", "t353", "t453", "t553", "t653",
  "t163", "t263", "t363", "t463", "t563", "t663",
  "t114", "t214", "t314", "t414", "t514", "t614",
  "t124", "t224", "t324", "t424", "t524", "t624",
  "t134", "t234", "t334", "t434", "t534", "t634",
  "t144", "t244", "t344", "t444", "t544", "t644",
  "t154", "t254", "t354", "t454", "t554", "t654",
  "t164", "t264", "t364", "t464", "t564", "t664",
  "t115", "t215", "t315", "t415", "t515", "t615",
  "t125", "t225", "t325", "t425", "t525", "t625",
  "t135", "t235", "t335", "t435", "t535", "t635",
  "t145", "t245", "t345", "t445", "t545", "t645",
  "t155", "t255", "t355", "t455", "t555", "t655",
  "t165", "t265", "t365", "t465", "t565", "t665",
  "t116", "t216", "t316", "t416", "t516", "t616",
  "t126", "t226", "t326", "t426", "t526", "t626",
  "t136", "t236", "t336", "t436", "t536", "t636",
  "t146", "t246", "t346", "t446", "t546", "t646",
  "t156", "t256", "t356", "t456", "t556", "t656",
  "t166", "t266", "t366", "t466", "t566", "t666", 
  " " /* blank terminates */
};


int ptc_twiss_summary_table_types[] =
  {
    2, 2, 2, 2, 2, 2, 2,
    2, 2, 2, 2, 2,
    2, 2, 2, 2,
    2,
    2,2,2,
    2,2,2,
    2,2,2,2,
    2,2,2,2
  };
char* ptc_twiss_summary_table_cols[] = {
  "length", "alpha_c", "alpha_c_p", "alpha_c_p2", "alpha_c_p3", "eta_c", "gamma_tr", 
  "q1", "q2", "dq1", "dq2", "qs",
  "beta_x_min","beta_x_max","beta_y_min","beta_y_max",
  "deltap",
  "orbit_x","orbit_px","orbit_y",
  "orbit_py","orbit_pt","orbit_-cT",
  "xcorms","ycorms","pxcorms","pycorms",
  "xcomax","ycomax","pxcomax","pycomax",
  " " /* blank terminates */
};

int ibs_table_types[] =
{
  3, 2, 2, 2, 2, 2
};

char* ibs_table_cols[] =
{
  "name", "s", "dels", "tli", "txi", "tyi",
  " "  /* blank terminates */
};

int map_tab_types[]=
{
  2,1,1,1,1,1,1,1,1,1
};

char* map_tab_cols[]=
{
  "coef","n_vector","nv","order","nx","nxp","ny","nyp","ndeltap","nt",
  " "  /* blank terminates */
};

int normal_res_types[] =
{
  3, 1, 1, 1, 1, 2
};

char* normal_res_cols[] =
{
  "name", "order1", "order2", "order3", "order4", "value",
  " "  /* blank terminates */
};

int sodd_detune_5_types[] =
{
  1, 1, 2, 1, 1
};

char* sodd_detune_5_cols[] =
{
  "multipoleorder", "plane", "detuning", "h_inv_order", "v_inv_order",
  " "  /* blank terminates */
};

int sodd_distort1_8_types[] =
{
  2, 2, 2, 2, 2, 2, 2, 2
};

char* sodd_distort1_8_cols[] =
{
  "multipoleorder", "cosine", "sine", "amplitude", "j", "k", "l", "m",
  " "  /* blank terminates */
};

int sodd_distort1_11_types[] =
{
  1, 1, 1, 2, 2, 2, 2, 1, 1, 1, 1
};

char* sodd_distort1_11_cols[] =
{
  "multipoleorder", "location", "resonance", "position[m]", "cosine", "sine", "amplitude", "j", "k", "l", "m",
  " "  /* blank terminates */
};

int sodd_distort2_9_types[] =
{
  1, 1, 2, 2, 2, 1, 1, 1, 1
};

char* sodd_distort2_9_cols[] =
{
  "multipoleorder1", "multipoleorder2", "cosine", "sine", "amplitude", "j", "k", "l", "m",
  " "  /* blank terminates */
};

int touschek_table_types[] =
{
  3, 2, 2, 2, 2
};

char* touschek_table_cols[] =
{
  "name", "s", "tli", "tliw", "tlitot",
  " "  /* blank terminates */
};

int mon_table_types[] =
{
  3, 2, 2, 2, 2
};

char* mon_table_cols[] =
{
  "name", "x.old", "y.old", "x", "y",
  " "  /* blank terminates */
};

int corr_table_types[] =
{
  3, 2, 2, 2, 2
};

char* corr_table_cols[] =
{
  "name", "px.old", "py.old", "px.correction", "py.correction",
  " "  /* blank terminates */
};

int orbit_table_types[] =
{
  3, 2, 2, 1,
};

char* orbit_table_cols[] =
{
  "name", "x", "y", "status",
  " "  /* blank terminates */
};

int special_comm_cnt[] =
{
  3, 5, 7, 6, 5, 4,
  0
};

char* special_comm_desc[] = /* ">?" = skip from start including char. at ? */
{
  "if(", "else{", "elseif(", "while(", ">:macro", ">:line",
  " "  /* blank terminates , line must remain last */
};

int summ_table_types[] =
{
  2, 2, 2, 2, 2,
  2, 2, 2, 2, 2,
  2, 2, 2, 2, 2,
  2, 2, 2, 2, 2,
  2, 2, 2, 2,
};

char* summ_table_cols[] =
{
  "length", "orbit5", "alfa", "gammatr", "q1",
  "dq1", "betxmax", "dxmax", "dxrms", "xcomax",
  "xcorms", "q2", "dq2", "betymax", "dymax",
  "dyrms", "ycomax", "ycorms", "deltap",
  "synch_1","synch_2","synch_3","synch_4","synch_5",
  " "  /* blank terminates */
};

int trackone_table_types[] =
{
  1, 1, 2, 2, 2, 2, 2, 2, 2, 2
};

char* trackone_table_cols[] =
{
  "number", "turn", "x", "px", "y", "py", "t", "pt", "s", "e",
  " "  /* blank terminates */
};

int track_table_types[] =
{
  1, 1, 2, 2, 2, 2, 2, 2, 2, 2
};

char* track_table_cols[] =
{
  "number", "turn", "x", "px", "y", "py", "t", "pt", "s", "e",
  " "  /* blank terminates */
};

int tracksumm_table_types[] =
{
  1, 1, 2, 2, 2, 2, 2, 2, 2, 2
};

char* tracksumm_table_cols[] =
{
  "number", "turn", "x", "px", "y", "py", "t", "pt", "s", "e",
  " "  /* blank terminates */
};


int ptcnodetrack_table_types[] =
{  1,        3,      1,         1,           1,      2,       2,   2,   2,    2,   2,    2,   2,    2 };

char* ptcnodetrack_table_cols[] =
{"number", "name", "elnumber","trnumber" , "turn","s_slice", "s", "x", "px", "y", "py", "t", "pt", "s",
 " "  /* blank terminates */
};


int trackloss_table_types[] =
{
  1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 3
};

char* trackloss_table_cols[] =
{
  "number", "turn", "x", "px", "y", "py", "t", "pt", "s", "e", "element",
  " "  /* blank terminates */
};

int dynap_table_types[] =
{
  2,2,2,2,2,
  2,2,2,2,2,
  2,2,2,2,2
};

char* dynap_table_cols[] =
{
  "dynapfrac", "dktrturns", "xend", "pxend", "yend",
  "pyend", "tend", "wxmin", "wxmax", "wymin", "wymax",
  "wxymin", "wxymax", "smear", "yapunov",
  " "  /* blank terminates */
};

int dynaptune_table_types[] =
{
  2,2,2,2,2
};

char* dynaptune_table_cols[] =
{
  "x", "y", "tunx", "tuny", "dtune",
  " "  /* blank terminates */
};

/* Definition of "select_ptc_normal" parameters for "ptc_normal" FS/VK 20.04.2006*/
char names[PTC_NAMES_L][5]=
{
  "dx","dpx","dy","dpy","q1","q2","dq1","dq2","anhx","anhy","haml","gnfu","eign"
};


char atm[3][4] = 
{
 "hmon","vmon","moni"
};

char atc[3][4] = 
{
 "hkic","vkic","kick"
};


char *atc_type = (char *)NULL;
int   atc_flag = 0;

char *atm_type = (char *)NULL;
int   atm_flag = 0;
