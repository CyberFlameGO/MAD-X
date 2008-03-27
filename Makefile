#######################################################################
#
# Makefile for MAD-X development version
#
#######################################################################

PLUGIN_SUPPORT=NO

# compilers
CC=gcc
FC=g77
# NAG for testing
#f95=f95
# LF95 for production
f95=lf95

# default fortran compiler options
FCP=-O4 -m32 -fno-second-underscore -funroll-loops -I.

# alternative for development and debug
FCM=-O2 -m32 -fno-second-underscore -funroll-loops
FCDB=-g -O0 -m32 -fno-second-underscore

# default C compiler flag options
GCCP_FLAGS_MPARS=-g -O4 -m32 -funroll-loops -D_CATCH_MEM -D_WRAP_FORTRAN_CALLS -I.
GCCP_FLAGS=$(GCCP_FLAGS_MPARS) -D_FULL
#to turn off fatal error at memory overflow add -D_DONOTCATCHOVERFLOW

# alternative for development
GCC_FLAGS=-g -m32 -Wall -D_CATCH_MEM -D_FULL -D_WRAP_FORTRAN_CALLS

# NAG default f95 compiler options
#f95_FLAGS=-gline -g90 -c -m32 -C=all -maxcontin=100 -nan
# NAG alternative
#f95_FLAGS=-c -O4 -m32 -maxcontin=100 -w=unused
# LF95 default f95 compiler options
f95_FLAGS= --o1 --tp -c -Wa,--32

# NAG f95 compiler options to compile f77 code
#FFLAGS77=-gline -g90 -c -m32 -maxcontin=100 -nan
# NAG f95 alternatives for development and debug
#FFLAGS77=-gline -g90 -c -m32 -maxcontin=100 -nan -ieee=full
#FFLAGS77=-g90 -c -O4 -m32 -maxcontin=100 -w=unused
# LF95 f95 compiler options to compile f77 code
FFLAGS77= --o1 --tp -c -Wa,--32 -D_G95

# g77 link options
FP=-static -m32

# NAG f95 link options
# LDOPT=-m32
# LF95 f95 link options
LDOPT=-static -m32

# libraries
#LIBX="-L/usr/X11R6/lib" -lX11 "-L/usr/lib/" -lgcc
#FC8
#LIBX= -lX11 -lXdmcp -lXau -lc -lpthread "-L/usr/lib/gcc/i386-redhat-linux/4.1.2" -lgcc -lgcc_eh
#LIBX="-L/usr/X11R6/lib64" -lX11 "-L/usr/lib64/" -ldl -lpthread
LIBX="-L/usr/X11R6/lib" -lX11 "-L/usr/lib/" -ldl -lpthread

# NAG f95 lib extension
#LIBX_ext= -lgcc
# LF95 f95 lib extension
LIBX_ext=

ifeq ($(PLUGIN_SUPPORT),YES)
  GCCP_FLAGS+= -DPLUGIN_SUPPORT
  #linker options to make dynamic linking
  #Lahey lf95
  LDOPT=--export -m32
  #g95
  #LDOPT=-rdynamic -m32
endif


ifeq ($(OSTYPE),darwin)
# allows running of madx under Macinstosh System 10
# -fno-second-underscore  is old, do not use for more recent gnu compilers
# include headers for gxx11c
  GCCP_FLAGS_MPARS=-g -O4 -m32 -funroll-loops -D_CATCH_MEM -I. -I /usr/X11R6/include/
  GCCP_FLAGS=$(GCCP_FLAGS_MPARS) -D_FULL
  FP=-m32
endif

default: madx

# dependencies of madxpf which combines the C-code
madxp.o: madxp.c madxn.c madxu.c aperture.c madxe.c madxc.c matchc.c matchc2.c sxf.c makethin.c c6t.c madxreg.c madxreg.h madx.h madxl.h madxd.h madxdict.h c6t.h matchptcknobs.h fortran_wrappers.h
	$(CC) $(GCCP_FLAGS_MPARS) -c madxp.c

madxpf.o: madxp.c madxn.c madxu.c aperture.c madxe.c madxc.c matchc.c matchc2.c sxf.c makethin.c c6t.c madxreg.c madxreg.h madx.h madxl.h madxd.h madxdict.h c6t.h matchptcknobs.h fortran_wrappers.h
	$(CC) $(GCCP_FLAGS) -c -o madxpf.o madxp.c


matchptcknobs.o: matchptcknobs.h matchptcknobs.c madx.h

# automatically generated code
fortran_wrappers.h:
	perl wrap_fortran_calls.pl 	# also generates fortran_wrappers.c, fortran_prototypes.h
					# and fortran_wrappers_prototypes.h

fortran_wrappers.o: fortran_wrappers.c
	$(CC) $(GCCP_FLAGS) -c fortran_wrappers.c

# fortran code dependencies on header files fi
twiss_f77.o twiss.o: twiss.F twiss0.fi twissa.fi twissl.fi twissc.fi twissotm.fi track.fi bb.fi name_len.fi twtrr.fi
util_f77.o util.o: util.F twiss0.fi twtrr.fi
dynap_f77.o dynap.o: dynap.F deltra.fi dyntab.fi wmaxmin0.fi tunes.fi
ibsdb_f77.o ibsdb.o: ibsdb.F ibsdb.fi name_len.fi physcons.fi
plot_f77.o plot.o: plot.F plot.fi plot_b.fi plot_c.fi plot_math.fi
sodd_f77.o sodd.o: sodd.F
trrun_f77.o trrun.o: trrun.F twiss0.fi name_len.fi track.fi bb.fi twtrr.fi
emit_f77.o emit.o: emit.F twiss0.fi bb.fi emit.fi twtrr.fi
match_f77.o match.o: match.F name_len.fi match.fi
touschek_f77.o touschek.o: touschek.F touschek.fi name_len.fi physcons.fi
resindex_f77.o resindex.o: resindex.F resindex.fi
fortran_flush_f77.o: fortran_flush.F

# f90 dependencies
a_scratch_size.o: a_scratch_size.f90
b_da_arrays_all.o: a_scratch_size.o b_da_arrays_all.f90
c_dabnew.o: b_da_arrays_all.o c_dabnew.f90
d_lielib.o: c_dabnew.o d_lielib.f90
h_definition.o: a_scratch_size.o c_dabnew.o d_lielib.o h_definition.f90
i_tpsa.o: h_definition.o i_tpsa.f90
j_tpsalie.o: i_tpsa.o j_tpsalie.f90
k_tpsalie_analysis.o: j_tpsalie.o k_tpsalie_analysis.f90
l_complex_taylor.o: k_tpsalie_analysis.o l_complex_taylor.f90
m_real_polymorph.o: l_complex_taylor.o m_real_polymorph.f90
n_complex_polymorph.o: m_real_polymorph.o n_complex_polymorph.f90
o_tree_element.o: n_complex_polymorph.o o_tree_element.f90
Sa_extend_poly.o: o_tree_element.o Sa_extend_poly.f90
Sb_sagan_pol_arbitrary.o: Sa_extend_poly.o Sb_sagan_pol_arbitrary.f90
Sc_euclidean.o: Sb_sagan_pol_arbitrary.o Sc_euclidean.f90
Sd_frame.o: Sc_euclidean.o Sd_frame.f90
Se_status.o: Sd_frame.o Se_status.f90 a_def_all_kind.inc a_def_sagan.inc \
	a_def_element_fibre_layout.inc
Sf_def_all_kinds.o: Se_status.o Sf_def_all_kinds.f90
Sg_sagan_wiggler.o: Sf_def_all_kinds.o Sg_sagan_wiggler.f90
Sh_def_kind.o: Sg_sagan_wiggler.o Sh_def_kind.f90
Si_def_element.o: Sh_def_kind.o Si_def_element.f90
Sk_link_list.o: Si_def_element.o Sk_link_list.f90
Sl_family.o: Sk_link_list.o Sl_family.f90
Sm_tracking.o: Sl_family.o Sm_tracking.f90
Sma_multiparticle.o: Sm_tracking.o Sma_multiparticle.f90
Sn_mad_like.o: Sma_multiparticle.o Sn_mad_like.f90
So_fitting.o: Sn_mad_like.o So_fitting.f90
Sp_keywords.o: So_fitting.o Sp_keywords.f90
Spb_fake_gino_sub.o: Sp_keywords.o Spb_fake_gino_sub.f90
Sq_orbit_ptc.o: Sp_keywords.o Sq_orbit_ptc.f90
Sqa_beam_beam_ptc.o: Sq_orbit_ptc.o Sqa_beam_beam_ptc.f90
Sqb_accel_ptc.o: Sqa_beam_beam_ptc.o Sqb_accel_ptc.f90
Sr_spin.o: Sqb_accel_ptc.o Sr_spin.f90
Sra_fitting.o: Sr_spin.o Sra_fitting.f90
madx_ptc_module.o: Sra_fitting.o madx_ptc_setcavs.o madx_ptc_knobs.o madx_ptc_module.f90 
St_pointers.o: Sp_keywords.o madx_ptc_module.o St_pointers.f90
madx_ptc_track_run.o: Sp_keywords.o madx_ptc_module.o madx_ptc_track_run.f90
madx_ptc_intstate.o: Sp_keywords.o madx_ptc_intstate.f90
madx_ptc_trackcavs.o: Sp_keywords.o madx_ptc_intstate.o  madx_ptc_setcavs.o madx_ptc_module.o madx_ptc_trackcavs.f90
madx_ptc_setcavs.o  : Sp_keywords.o madx_ptc_intstate.o  madx_ptc_setcavs.f90
madx_ptc_script.o  : Sp_keywords.o madx_ptc_script.f90
madx_ptc_knobs.o : Sp_keywords.o madx_ptc_intstate.o madx_ptc_knobs.f90
madx_ptc_eplacement.o  : Sp_keywords.o madx_ptc_intstate.o madx_ptc_module.o madx_ptc_eplacement.f90
madx_ptc_normal.o: madx_ptc_module.o madx_ptc_normal.f90
madx_ptc_twiss.o: madx_ptc_module.o madx_ptc_setcavs.o madx_ptc_knobs.o madx_ptc_distrib.o madx_ptc_twiss.f90
madx_ptc_distrib.o: madx_ptc_module.o madx_ptc_distrib.f90

wrap.o: madx_ptc_module.o  madx_ptc_intstate.o \
	madx_ptc_normal.o madx_ptc_twiss.o madx_ptc_distrib.o \
	madx_ptc_setcavs.o madx_ptc_trackcavs.o \
	madx_ptc_knobs.o \
	madx_ptc_script.o St_pointers.o \
	wrap.f90
user2_photon.o: madx_ptc_track_run.o user2_photon.f90 photoni.inc
run_madx.o: madx_ptc_module.o run_madx.f90
madx_main.o: run_madx.o madx_main.f90

# matchlib2 for madx only
matchlib2_f77.o: matchlib2.F
	$(FC) -m32 -c -o $@ $<

# implicit rule to compile with C
%.o : %.c
	$(CC) $(GCCP_FLAGS) -c -o $(@) $<

# implicit rule to compile with f77. Append _f77 to distinguish from object code compiled with f95
%_f77.o : %.F
	$(FC) $(FCP) -c -o $(@) $<

# implicit rule to compile f77 code with f95
%.o : %.F
	$(f95) $(FFLAGS77) $<

# implicit rule to compile f90 code with f95
%.o : %.f90
	$(f95) $(f95_FLAGS) $<


#Parser only
mpars: madxm.F madxp.o
	$(FC) $(FP) -o mpars madxm.F madxp.o $(LIBX) -lm -lc

# madx_objectsf77: madxpf.o gxx11c.o  + all *.F except for gxx11ps.F timest.F timex.F (windows special & F90).
# Append f77 to distinguish from objects compiled with f95
madx_objectsf77 = madxpf.o gxx11c.o timel.o matchptcknobs.o fortran_wrappers.o\
	$(filter-out gxx11ps_f77.o madxp.o, $(patsubst %.F,%_f77.o,$(wildcard *.F)))

madx: $(madx_objectsf77);
	$(FC) $(FP) -o $@ $(madx_objectsf77) $(LIBX) -lgcc -lm -lc

# madx_objectsf95 all *.F without madxm.F, ptc_dummy.F & gxx11ps.F (windows special)
madx_objectsf95 = $(filter-out madxm.o ptc_dummy.o gxx11ps.o madxp.o matchlib2_f77.o matchlib2.o, $(patsubst %.F,%.o,$(wildcard *.F)))
# madxp_objects. All *.f90 , some c and F
madxp_objects = $(patsubst %.f90,%.o,$(wildcard *.f90)) \
	madxpf.o gxx11c.o matchptcknobs.o rplot.o fortran_wrappers.o\
	$(madx_objectsf95)
madxp: $(madxp_objects)
	$(f95) $(LDOPT) -o $@ $(madxp_objects) $(LIBX) $(LIBX_ext)

clean:
	rm -f *.o
	rm -f *.g90
	rm -f *.mod
	rm -f core
	rm -f *~
	rm -f fortran_wrappers.c fortran_wrappers.h
	rm -f fortran_prototypes.h fortran_wrappers_prototypes.h

info:
	@echo "-------------------------------------"
	@echo  Makefile for madX by Helmut Burkhardt
	@echo "-------------------------------------"
	@echo madx_objectsf77 = $(sort $(madx_objectsf77))
	@echo
	@echo madxp_objects = $(sort $(madxp_objects))
	@echo
	@echo default C compiler CC "    " = $(CC)
	@echo GCC_FLAGS "                " = $(GCC_FLAGS)
	@echo GCCP_FLAGS "               " = $(GCCP_FLAGS)
	@echo default Fortran compiler FC  = $(FC)
	@echo FFLAGS77 "                 " = $(FFLAGS77)
	@echo f95 "                      " = $(f95)
	@echo FCP "                      " = $(FCP)
	@echo FCM "                      " = $(FCM)
	@echo FCDB "                     " = $(FCDB)
	@echo LIBX "                     " = $(LIBX)
	@echo GLIB "                     " = $(GLIB)
	@echo GPUB "                     " = $(GPUB)
	@echo the OS is "                " = $(OS)
	@echo the OSTYPE is "            " = $(OSTYPE)
