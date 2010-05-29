MODULE ptc_results
  USE madx_keywords
  implicit none
  public
  integer :: number_variables = 6
  integer :: order = 20
  character(len = 2), dimension(6) :: ptc_variables = (/'x ','xp','y ','yp','z ','dp'/)
  character(len = 2) :: ptc_var
  type(normalform) n
  type (pbresonance) pbrg,pbrh
END MODULE ptc_results

MODULE madx_ptc_module
  use S_fitting_new
  !  USE madx_keywords
  USE madx_ptc_setcavs_module
  USE madx_ptc_knobs_module
  use madx_ptc_intstate_module, only : getdebug

  implicit none
  public
  logical(lp) mytime
  integer icav

  integer :: universe=0,index_mad=0,EXCEPTION=0
  integer ipause
  integer,external :: mypause
  real(kind(1d0)) get_value,node_value
  type(layout),pointer :: MY_RING
  type(mad_universe),target ::  m_u
  integer, private, parameter :: mynreso=20
  integer, private, dimension(4) :: iia,icoast
  real(dp) :: mux_default=c_0_28, muy_default=c_0_31, muz_default=c_1d_3
  integer, private, allocatable :: J(:)
  logical(lp)             :: savemaps=.false.
  logical(lp) :: resplit,even
  real(dp) my_thin,my_xbend

  type mapbuffer
     type(universal_taylor)  :: unimap(6)
     real(dp)                :: s
     character(nlp+1)        :: name
  end type mapbuffer

  type(mapbuffer), pointer  :: maps(:) !buffered maps from the last twiss
  integer                   :: mapsorder = 0  !order of the buffered maps, if 0 maps no maps buffered
  integer                   :: mapsicase = 0

CONTAINS

  subroutine ptc_create_universe()
    implicit none
    real(kind(1d0)) get_value

    print77=.false.
    read77 =.false.
    lingyun_yang=get_value('ptc_create_universe ','ntpsa ').ne.0
    lielib_print(6)=get_value('ptc_create_universe ','symprint ')

    nullify(maps)

    if (getdebug()==0) global_verbose = .false.
    if (getdebug()>0) print*,"Now PTC"
    sector_nmul_max = get_value('ptc_create_universe ','sector_nmul_max ')

    !    print*,">>ss1<< old sector_nmul",sector_nmul

    sector_nmul = get_value('ptc_create_universe ','sector_nmul ')

    !    print*,">>ss1<< new sector_nmul",sector_nmul

    if(sector_nmul_max.lt.sector_nmul) then
       call aafail('sector_nmul_max must be larger than sector_nmul: ',&
            'check your ptc_create_universe input')
    endif
    call set_up_universe(m_u)
    universe=universe+1


  end subroutine ptc_create_universe
  !_________________________________________________________________

  subroutine ptc_create_layout()
    implicit none
    real(kind(1d0)) get_value

    if(universe.le.0) then
       call fort_warn('return from ptc_create_layout: ',' no universe created')
       return
    endif

    call append_empty_layout(m_u)
    index_mad=index_mad+1
    my_ring=>m_u%end

    call ptc_input()

    if(EXCEPTION.eq.1) then
       call fort_warn('wrong magnet type KINDI which must be: ','1, 2, 3')
       return
    endif

    cavsareset = .false.
    mytime=get_value('ptc_create_layout ','time ').ne.0

    if(mytime) then
       default=getintstate()
       default=default+time
       call setintstate(default)
    endif

  end subroutine ptc_create_layout
  !_________________________________________________________________

  subroutine ptc_move_to_layout()
    implicit none
    real(kind(1d0)) get_value
    integer my_index

    if(universe.le.0) then
       call fort_warn('return from ptc_move_to_layout: ',' no universe created')
       return
    endif

    my_index = get_value('ptc_move_to_layout ','index ')

    if(my_index.gt.index_mad.or.my_index.le.0) then
       call fort_warn('return from ptc_move_to_layout: ',' layout outside allowed range')
       print*,"   Allowed range 0 < ",index_mad
       return
    endif

    call move_to_layout_i(m_u,my_ring,my_index)

  end subroutine ptc_move_to_layout
  !_________________________________________________________________

  subroutine ptc_input()
    use twtrrfi
    use twiss0fi
    use name_lenfi
    implicit none
    logical(lp) particle,doneit,isclosedlayout
    integer i,j,k,code,nt,icount,nn,ns,nd,mg,get_string
    !    integer get_option
    integer double_from_table
    integer restart_sequ,advance_node,n_ferr,node_fd_errors
    integer, parameter :: nt0=20000
    real(dp) l,l_machine,energy,kin,brho,beta0,p0c,pma,e0f,lrad,charge
    real(dp) f_errors(0:maxferr),aperture(maxnaper),normal(0:maxmul)
    real(dp) patch_ang(3),patch_trans(3)
    real(dp) skew(0:maxmul),field(2,0:maxmul),fieldk(2),myfield(2*maxmul+2)
    real(dp) gamma,gamma2,gammatr2,freq,offset_deltap
    real(dp) fint,fintx,div,muonfactor,edge,rhoi,hgap,corr,tanedg,secedg,psip
    real(dp) sk1,sk1s,sk2,sk2s,sk3,sk3s,tilt,dum1,dum2
    REAL(dp) ::  normal_0123(0:3), skew_0123(0:3) ! <= knl(1), ksl(1)
    real(dp) gammatr,ks,ksi
    real(kind(1d0)) get_value,node_value
    character(name_len) name
    character(name_len) aptype
    type(keywords) key
    character(20)       keymod0,keymod1
    character(name_len) magnet_name
    logical(lp)         exact0
    integer             exact1
    integer             sector_nmul_max0,sector_nmul0
    integer             model
    integer             method0,method1
    integer             nst0,nst1,ord_max,kk
    REAL (dp) :: tempdp,bvk
    logical(lp):: ptcrbend,truerbend,errors_out
    !  Etienne helical
    character(nlp) heli(100)
    integer mheli,helit,ihelit
    type(fibre), pointer :: p
    !---------------------------------------------------------------
    !---------------------------------------------------------------
    if (getdebug() > 1) then
       print *, '--------------------------------------------------------------'
       print *, '--------------------------------------------------------------'
       print *, '------    E X E C U T I N G     P T C     I N P U T   --------'
       print *, '--------------------------------------------------------------'
       print *, '--------------------------------------------------------------'
    endif

    energy=get_value('probe ','energy ')
    pma=get_value('probe ','mass ')
    charge=get_value('probe ','charge ')
    bvk=get_value('probe ','bv ')

    e0f=sqrt(ENERGY**2-pma**2)

    if (getdebug() > 0) then
       print *, 'MAD-X Beam Parameters'
       print '(a26, e13.6)', '      Energy :',energy
       print '(a26, e13.6)', '      Kinetic Energy :',energy-pma
       print '(a26, e13.6)', '      Particle Rest Mass :',pma
       print '(a26, e13.6)', '      Momentum :',e0f
    endif



    beta0=e0f/ENERGY


    if(abs(pma-pmae)/pmae<c_0_002) then
       if (getdebug() > 1) print *,'Executing MAKE_STATES(TRUE), i.e. ELECTRON beam'
       particle=.true.
       CALL MAKE_STATES(PARTICLE)
    elseif(abs(pma-pmap)/pmap<c_0_002) then
       if (getdebug() > 1) print *,'Executing MAKE_STATES(FALSE), i.e. PROTON beam'
       particle=.false.
       CALL MAKE_STATES(PARTICLE)
    else
       if (getdebug() > 1) print '(a, f8.4, a)','Executing MAKE_STATES(',pma/pmae,'), i.e. PROTON beam'
       muonfactor=pma/pmae
       CALL MAKE_STATES(muonfactor)
    endif

    !the state is cleared at this stage
    call setintstate(default)
    !valid October 2002: oldscheme=.false.
    !!valid October 2002: oldscheme=.true.

    if (getdebug()==0) global_verbose = .false.

    !  with_external_frame=.false.
    !  with_internal_frame=.false.
    !  with_chart=.false.
    !  with_patch=.false.

    ! Global Keywords

    if (getdebug() > 1) then
       print *, '=============================================================='
       print *, 'INPUT PARAMETERS ARE:'
    endif

    sector_nmul_max0 = sector_nmul_max
    if (getdebug() > 1) print*,'  Global max sector_nmul: ',sector_nmul_max0

    sector_nmul0 = sector_nmul
    if (getdebug() > 1) print*,'  Global sector_nmul: ',sector_nmul0


    model = get_value('ptc_create_layout ','model ')
    if (getdebug() > 1) print*,'  Global Model code is : ',model

    !*****************************
    !  MODEL Settings
    !*****************************
    select case(model)
    CASE(1)
       keymod0 = "DRIFT_KICK       "
    CASE(2)
       keymod0 = "MATRIX_KICK      "
    CASE(3)
       keymod0 = "DELTA_MATRIX_KICK"
    CASE DEFAULT
       PRINT *, 'EXCEPTION occured: Can not recognize model type ',model
       EXCEPTION=1
       ipause=mypause(444)
       RETURN
    END SELECT



    if (getdebug() > 1) print*,'  Global Model name (keymod0) is : ',keymod0

    method0   = get_value('ptc_create_layout ','method ')
    if (getdebug() > 1) print*,'  Global method is: ',method0

    exact0    = get_value('ptc_create_layout ','exact ') .ne. 0
    if (getdebug() > 1) print*,'  Global exact is: ',exact0

    nst0      = get_value('ptc_create_layout ','nst ')
    if (getdebug() > 1) print*,'  Global Number of Integration Steps (nst) is: ',nst0

    ! MAD-X specials
    !    madlength = get_option('rbarc ') .eq. 0
    madlength = .false.
    if (getdebug() > 1) print*,'  global rbend_length: ',madlength

    mad       = get_value('ptc_create_layout ','mad_mult ') .ne. 0
    if (getdebug() > 1) print*,'  global mad_mult as in mad8: ',mad

    mad8      = get_value('ptc_create_layout ','mad8 ') .ne. 0
    if (getdebug() > 1) print*,'  rbend as in mad8 (only global): ',mad8

    gamma     = get_value('probe ','gamma ')
    if (getdebug() > 1) print*,'  gamma: ',gamma

    k         = double_from_table('summ ','gammatr ',1,gammatr)
    if (getdebug() > 1) print*,'  gammatr: ',gammatr

    gamma2    = gamma**2
    gammatr2  = gammatr**2

    if (getdebug() > 1) then
       print *, '=============================================================='
       print *, ''
    endif

    !  call Set_Up(MY_RING)

    if (getdebug() > 0) then
       print *, 'Setting MADx with '
       print *, '    energy        ',energy
       print *, '    method        ',method0
       print *, '    Num. of steps ',nst0
       print *, '    charge        ',charge
    endif
    ! etienne helical
    helit=0
    call kanalnummer(mheli)
    open(unit=mheli,file='helical.txt',status='OLD',err=1001)
    read(mheli,*) helit
    if(helit>100) then
       write(6,*) " too many helical dipole ",helit
       stop 99
    endif
    do ihelit=1,helit
       read(mheli,*) heli(ihelit)
       CALL CONTEXT(heli(ihelit))
    enddo
    close(mheli)
1001 continue
    helit=0
    call kanalnummer(mheli)
    open(unit=mheli,file='sixtrack_compatible.txt',status='OLD',err=1002)
    read(mheli,*) sixtrack_compatible
    close(mheli)
1002 continue
    ! end of etienne helical

    ! preliminary setting
    !    my_ring%charge=1
    initial_charge=1
    CALL SET_MADx(energy=energy,METHOD=method0,STEP=nst0)
    if (getdebug() > 1) print *, 'MADx is set'

    icav=0
    nt=0
    j=restart_sequ()
    j=0
    l_machine=zero

    errors_out = get_value('ptc_create_layout ','errors_out ').ne.0
    magnet_name=" "
    if(errors_out) mg = get_string('ptc_create_layout ','magnet_name ',magnet_name)

10  continue
    nst1=node_value("nst ")
    if(nst1.gt.0) then
       nstd = nst1
    else
       nstd = nst0
    endif

    call zero_key(key)

    j=j+1
    nt=nt+1
    if(nt==nt0) then
       call fort_warn("Potential problem for very large structure: ","More than 20'000 elements found")
    endif
    icount=0
    l=zero
    l=node_value('l ')
    key%list%l=l
    l_machine=l_machine+l
    code=node_value('mad8_type ')
    if(code.eq.39) code=15
    if(code.eq.38) code=24
    call element_name(name,name_len)
    key%list%name=name

    call node_name(name,name_len)
    key%list%vorname=name

    !frs&piotr 18 Dec 2007: sector_nmul must stay global for the time being
    !local, if present, superseed global at current node


    !*****************************
    !  MODEL Settings
    !*****************************

    model = node_value('model ')
    keymod1 = " "
    select case(model)
    CASE(1)
       keymod1 = "DRIFT_KICK       "
    CASE(2)
       keymod1 = "MATRIX_KICK      "
    CASE(3)
       keymod1 = "DELTA_MATRIX_KICK"
    END SELECT


    if(keymod1.ne." ") then
       key%model=keymod1
    else
       key%model=keymod0
    endif
    method1=node_value("method ")
    if(method1.eq.2.or.method1.eq.4.or.method1.eq.6) then
       metd = method1
    else
       metd = method0
    endif

    exact1=node_value("exact ")

    if(exact1.eq.0.or.exact1.eq.1) then
       EXACT_MODEL = exact1 .ne. 0
    else
       EXACT_MODEL = exact0
    endif

    !special node keys
    key%list%permfringe=node_value("permfringe ") .ne. zero
    key%list%kill_ent_fringe=node_value("kill_ent_fringe ") .ne. zero
    key%list%kill_exi_fringe=node_value("kill_exi_fringe ") .ne. zero
    key%list%bend_fringe=node_value("bend_fringe ") .ne. zero

    nn=name_len
    call node_string('apertype ',aptype,nn)
    call dzero(aperture,maxnaper)
    call get_node_vector('aperture ',nn,aperture)
    if(.not.((aptype.eq."circle".and.aperture(1).eq.zero).or.aptype.eq." ")) then
       c_%APERTURE_FLAG=.true.
       select case(aptype)
       case("circle")
          key%list%aperture_on=.true.
          key%list%aperture_kind=1
          key%list%aperture_r(1)=aperture(1)
          key%list%aperture_r(2)=aperture(1)
       case("ellipse")
          key%list%aperture_on=.true.
          key%list%aperture_kind=1
          key%list%aperture_r(1)=aperture(1)
          key%list%aperture_r(2)=aperture(2)
       case("rectangular")
          key%list%aperture_on=.true.
          key%list%aperture_kind=2
          key%list%aperture_x=aperture(1)
          key%list%aperture_y=aperture(2)
          !       case("lhcscreen")
       case("rectellipse")
          key%list%aperture_on=.true.
          key%list%aperture_kind=3
          key%list%aperture_x=aperture(1)
          key%list%aperture_y=aperture(2)
          key%list%aperture_r(1)=aperture(3)
          key%list%aperture_r(2)=aperture(4)
       case("marguerite")
          key%list%aperture_on=.true.
          key%list%aperture_kind=4
          key%list%aperture_r(1)=aperture(1)
          key%list%aperture_r(2)=aperture(2)
       case("racetrack")
          key%list%aperture_on=.true.
          key%list%aperture_kind=5
          key%list%aperture_x=aperture(1)
          key%list%aperture_y=aperture(2)
          key%list%aperture_r(1)=aperture(3)
       case("general")
          key%list%aperture_kind=6
          print*,"General aperture not implemented"
          stop
       end select
    endif
    call append_empty(my_ring)

    select case(code)
    case(0,4,25)
       key%magnet="marker"
    case(1,11,20,21)
       key%magnet="drift"
       CALL CONTEXT(key%list%name)

       do ihelit=1,helit
          IF(index(key%list%name,heli(ihelit)(1:len_trim(heli(ihelit))))/=0) then
             key%magnet="helicaldipole"
             write(6,*) " drift ",key%list%name, " became helical dipole in PTC "
          endif
       enddo
       ! end etienne Helical
    case(2) ! PTC accepts mults
       if(l.eq.zero) then
          key%magnet="marker"
          goto 100
       endif
       key%magnet="rbend"
       !VK
       CALL SUMM_MULTIPOLES_AND_ERRORS (l, key, normal_0123,skew_0123,ord_max)

       tempdp=sqrt(normal_0123(0)*normal_0123(0)+skew_0123(0)*skew_0123(0))
       key%list%b0=bvk*(node_value('angle ')+tempdp*l)

       !       print*, "RBEND: Angle: ", node_value('angle ')," tempdp ", tempdp, " l ", l
       !       print*, "RBEND: normal: ",normal_0123(0)," skew: ",skew_0123(0)

       key%list%k(2)=node_value('k1 ')+ key%list%k(2)
       key%list%k(3)=node_value('k2 ')+ key%list%k(3)
       key%list%k(4)=node_value('k3 ')+ key%list%k(4)

       key%list%ks(2)=node_value('k1s ')+ key%list%ks(2)
       key%list%ks(3)=node_value('k2s ')+ key%list%ks(3)
       key%list%ks(4)=node_value('k3s ')+ key%list%ks(4)

       if(EXACT_MODEL.and.(node_value('angle ').eq.zero)) then
          key%magnet="quadrupole"
          key%tiltd=node_value('tilt ')
       else

          ! Gymnastic needed since PTC expects MAD8 convention
          key%list%t1=node_value('e1 ')
          key%list%t2=node_value('e2 ')
          key%list%hgap=node_value('hgap ')
          !       key%list%fint=node_value('fint ')
          fint=node_value('fint ')
          fintx=node_value('fintx ')
          if((fintx.ne.fint).and.(fintx.gt.zero.and.fint.gt.zero)) then
             print*," The fint and fintx must be the same at each end or each might be zero"
             stop
          endif
          if(fint.gt.zero) then
             key%list%fint=fint
             if(fintx.eq.zero) key%list%kill_exi_fringe=my_true
          else
             if(fintx.gt.zero) then
                key%list%fint=fintx
                key%list%kill_ent_fringe=my_true
             else
                key%list%fint=zero
             endif
          endif
          key%list%h1=node_value('h1 ')
          key%list%h2=node_value('h2 ')
          key%tiltd=node_value('tilt ')
          if(tempdp.gt.0) key%tiltd=key%tiltd + atan2(skew_0123(0),normal_0123(0))
          ptcrbend=node_value('ptcrbend ').ne.0
          if(ptcrbend) then
             call context(key%list%name)
             truerbend=node_value('truerbend ').ne.0
             if(truerbend) then
                key%magnet="TRUERBEND"
                if(key%list%t2/=zero) then
                   write(6,*) " The true parallel face bend "
                   write(6,*) " only accepts the total angle and e1 as an input "
                   write(6,*) " if e1=0, then the pipe angle to the entrance face is "
                   write(6,*) " angle/2. It is a normal rbend."
                   write(6,*) " If e1/=0, then the pipe angle to the entrance face is "
                   write(6,*) ' angle/2+e1 and the exit pipe makes an angle "angle/2-e1" '
                   write(6,*) " with the exit face."
                   write(6,*) " The offending non-zero t2 = (e2 - angle/2) is set to zero! "
                   write(6,*) " Make sure that this is what you want!!! "
                   !                write(6,*) " CHANGE YOUR LATTICE FILRE."
                   !                stop 666
                   key%list%t2=zero
                endif
             else
                key%magnet="WEDGRBEND"
             endif
          endif
       endif
       if(errors_out) then
          if(key%list%name(:len_trim(magnet_name)-1).eq. &
               magnet_name(:len_trim(magnet_name)-1)) then
             call string_to_table('errors_dipole ', 'name ',key%list%name)
             call double_to_table('errors_dipole ', 'k0l ',bvk*key%list%b0)
             call augment_count('errors_dipole ')
          endif
       endif
    case(3) ! PTC accepts mults watch out sector_nmul defaulted to 4
       if(l.eq.zero) then
          key%magnet="marker"
          goto 100
       endif
       key%magnet="sbend"
       !VK
       CALL SUMM_MULTIPOLES_AND_ERRORS (l, key, normal_0123,skew_0123,ord_max)
       if(sector_nmul_max.lt.ord_max.and.EXACT_MODEL) call aafail('the order of multipoles in a sbend in exact mode cannot be ',&
            &'larger than sector_mul_max: check your ptc_create_universe input')

       tempdp=sqrt(normal_0123(0)*normal_0123(0)+skew_0123(0)*skew_0123(0))
       key%list%b0=bvk*(node_value('angle ')+ tempdp*l)

       key%list%k(2)=node_value('k1 ')+ key%list%k(2)
       key%list%k(3)=node_value('k2 ')+ key%list%k(3)
       key%list%k(4)=node_value('k3 ')+ key%list%k(4)

       key%list%ks(2)=node_value('k1s ')+ key%list%ks(2)
       key%list%ks(3)=node_value('k2s ')+ key%list%ks(3)
       key%list%ks(4)=node_value('k3s ')+ key%list%ks(4)

       key%list%t1=node_value('e1 ')
       key%list%t2=node_value('e2 ')
       key%list%hgap=node_value('hgap ')
       !       key%list%fint=node_value('fint ')
       fint=node_value('fint ')
       fintx=node_value('fintx ')
       if((fintx.ne.fint).and.(fintx.gt.zero.and.fint.gt.zero)) then
          print*," The fint and fintx must be the same at each end or each might be zero"
          stop
       endif
       if(fint.gt.zero) then
          key%list%fint=fint
          if(fintx.eq.zero) key%list%kill_exi_fringe=my_true
       else
          if(fintx.gt.zero) then
             key%list%fint=fintx
             key%list%kill_ent_fringe=my_true
          else
             key%list%fint=zero
          endif
       endif
       key%list%h1=node_value('h1 ')
       key%list%h2=node_value('h2 ')
       key%tiltd=node_value('tilt ')
       if(tempdp.gt.0) key%tiltd=key%tiltd + atan2(skew_0123(0),normal_0123(0))
       if(errors_out) then
          if(key%list%name(:len_trim(magnet_name)-1).eq. &
               magnet_name(:len_trim(magnet_name)-1)) then
             call string_to_table('errors_dipole ', 'name ',key%list%name)
             call double_to_table('errors_dipole ', 'k0l ',bvk*key%list%b0)
             call augment_count('errors_dipole ')
          endif
       endif
    case(5)
       key%magnet="quadrupole"
       !VK
       CALL SUMM_MULTIPOLES_AND_ERRORS (l, key, normal_0123,skew_0123,ord_max)

       ! Read data & fill %k(:), %ks(:) arrays which are
       ! summs of multipoles and errors

       ! quadrupole components
       sk1= node_value('k1 ')
       sk1s=node_value('k1s ')
       tilt=node_value('tilt ')
       dum1=key%list%k(2)-normal_0123(1)
       dum2=key%list%ks(2)-skew_0123(1)

       if(dum1.ne.zero.or.dum2.ne.zero) then                      !
          sk1= sk1 +dum1                                          !
          sk1s=sk1s+dum2                                          !
       endif                                                      !
       if (sk1s .ne. zero) then                                   !
          tilt = -atan2(sk1s, sk1)/two + tilt                     !
          sk1 = sqrt(sk1**2 + sk1s**2)                            !
       endif                                                      !
       key%list%k(2) =sk1                                         !
       key%list%ks(2)=zero  ! added by VK                         !
       key%tiltd=tilt  !==========================================!

       !================================================================

    case(6)
       key%magnet="sextupole"
       !VK
       CALL SUMM_MULTIPOLES_AND_ERRORS (l, key, normal_0123,skew_0123,ord_max)

       ! sextupole components
       sk2= node_value('k2 ')
       sk2s=node_value('k2s ')
       tilt=node_value('tilt ')
       dum1=key%list%k(3)-normal_0123(2)
       dum2=key%list%ks(3)-skew_0123(2)

       if(dum1.ne.zero.or.dum2.ne.zero) then                      !
          sk2= sk2 +dum1                                          !
          sk2s=sk2s+dum2                                          !
       endif                                                      !
       if (sk2s .ne. zero) then                                   !
          tilt = -atan2(sk2s, sk2)/three + tilt                   !
          sk2 = sqrt(sk2**2 + sk2s**2)                            !
       endif                                                      !
       key%list%k(3) =sk2                                         !
       key%list%ks(3)=zero  ! added by VK                         !
       key%tiltd=tilt  !==========================================!

       !================================================================

    case(7)
       key%magnet="octupole"
       !VK
       CALL SUMM_MULTIPOLES_AND_ERRORS (l, key, normal_0123,skew_0123,ord_max)

       ! octupole components
       sk3= node_value('k3 ')
       sk3s=node_value('k3s ')

       tilt=node_value('tilt ')
       dum1=key%list%k(4)-normal_0123(3)
       dum2=key%list%ks(4)-skew_0123(3)

       if(dum1.ne.zero.or.dum2.ne.zero) then                      !
          sk3= sk3 +dum1                                          !
          sk3s=sk3s+dum2                                          !
       endif                                                      !
       if (sk3s .ne. zero) then                                   !
          tilt = -atan2(sk3s, sk3)/four + tilt                    !
          sk3 = sqrt(sk3**2 + sk3s**2)                            !
       endif                                                      !
       key%list%k(4) =sk3                                         !
       key%list%ks(4)=zero  ! added by VK                         !

       key%tiltd=tilt  !==========================================!

       !================================================================

    case(8)
       key%magnet="multipole"
       !---- Multipole components.
       call dzero(f_errors,maxferr+1)
       n_ferr = node_fd_errors(f_errors)
       call dzero(normal,maxmul+1)
       call dzero(skew,maxmul+1)
       call get_node_vector('knl ',nn,normal)
       call get_node_vector('ksl ',ns,skew)
       if(nn.ge.NMAX) nn=NMAX-1
       if(ns.ge.NMAX) ns=NMAX-1
       do i=1,NMAX
          key%list%k(i)=zero
          key%list%ks(i)=zero
       enddo
       skew(0)=-skew(0) ! frs error found 30.08.2008
       key%list%thin_h_angle=bvk*normal(0)
       key%list%thin_v_angle=bvk*skew(0)
       lrad=node_value('lrad ')
       if(lrad.gt.zero) then
          key%list%thin_h_foc=normal(0)*normal(0)/lrad
          key%list%thin_v_foc=skew(0)*skew(0)/lrad
       endif
       if(nn.gt.0) then
          do i=1,nn
             key%list%k(i+1)=normal(i)
          enddo
       endif
       if(ns.gt.0) then
          do i=1,ns
             key%list%ks(i+1)=skew(i)
          enddo
       endif
       call dzero(field,2*(maxmul+1))
       if (n_ferr .gt. 0) then
          call dcopy(f_errors,field,n_ferr)
       endif
       nd = max(nn, ns, n_ferr/2)
       if(nd.ge.maxmul) nd=maxmul-1
       if(n_ferr.gt.0) then
          do i=0,nd
             key%list%k(i+1)=key%list%k(i+1)+field(1,i)
             key%list%ks(i+1)=key%list%ks(i+1)+field(2,i)
          enddo
       endif
       key%tiltd=node_value('tilt ')
       if(errors_out) then
          if(key%list%name(:len_trim(magnet_name)-1).eq. &
               magnet_name(:len_trim(magnet_name)-1)) then
             call string_to_table('errors_field ', 'name ',key%list%name)
             call string_to_table('errors_total ', 'name ',key%list%name)
             i=2*maxmul+2
             myfield(:) = zero
             do kk=1,nd+1
                myfield(2*kk-1) = field(1,kk-1)
                myfield(2*kk)   = field(2,kk-1)
             enddo
             call vector_to_table('errors_field ', 'k0l ', i, myfield(1))
             myfield(:) = zero
             do kk=1,nd+1
                myfield(2*kk-1) = key%list%k(kk)
                myfield(2*kk)   = key%list%ks(kk)
             enddo
             call vector_to_table('errors_total ', 'k0l ', i, myfield(1))
             call augment_count('errors_field ')
             call augment_count('errors_total ')
          endif
       endif
    case(9) ! PTC accepts mults
       key%magnet="solenoid"
       ks=node_value('ks ')
       if(l.ne.zero) then
          key%list%bsol=bvk*ks
       else
          ksi=node_value('ksi ')
          lrad=node_value('lrad ')
          if(lrad.eq.zero.and.ks.ne.zero) lrad=ksi/ks
          if(ksi.eq.zero.or.lrad.eq.zero) then
             key%magnet="marker"
             print*,"Thin solenoid: ",name," has no strength - set to marker"
          else
             key%list%bsol=bvk*ksi/lrad
             key%list%ls=lrad
          endif
       endif
       !VK
       CALL SUMM_MULTIPOLES_AND_ERRORS (l, key, normal_0123,skew_0123,ord_max)

    case(10)
       key%magnet="rfcavity"
       key%list%volt=bvk*node_value('volt ')
       freq=c_1d6*node_value('freq ')
       key%list%lag=node_value('lag ')*twopi
       offset_deltap=get_value('ptc_create_layout ','offset_deltap ')
       if(offset_deltap.ne.zero) then
          default = getintstate()
          default=default+totalpath0
          call setintstate(default)
          freq=freq*((gammatr2-gamma2)*offset_deltap/gammatr2/gamma2+one)
       endif
       key%list%freq0=freq
       key%list%n_bessel=node_value('n_bessel ')
       key%list%harmon=one
       if(key%list%volt.ne.zero.and.key%list%freq0.ne.zero) icav=1
       !  case(11)
       !     key%magnet="elseparator"
       !     key%list%volt=node_value('ex ')
       !     key%list%lag=atan2(node_value('ey '),node_value('ex '))
       !     key%tiltd=node_value('tilt ')
       m_u%end%HARMONIC_NUMBER=node_value('harmon ')   ! etienne_harmon
    case(12)
       ! actually our SROT element
       key%magnet="CHANGEREF"
       call dzero(patch_ang,3)
       call dzero(patch_trans,3)
       patch_ang(3)=node_value('angle ')
       key%list%patchg=2
       do i=1,3
          key%list%ang(i)=patch_ang(i)
          key%list%t(i)=patch_trans(i)
       enddo
    case(13)
       ! actually our YROT element
       key%magnet="CHANGEREF"
       call dzero(patch_ang,3)
       call dzero(patch_trans,3)
       patch_ang(2)=-node_value('angle ')
       key%list%patchg=2
       do i=1,3
          key%list%ang(i)=patch_ang(i)
          key%list%t(i)=patch_trans(i)
       enddo
    case(14,15,16) ! PTC accepts mults
       call dzero(f_errors,maxferr+1)
       n_ferr = node_fd_errors(f_errors)
       do i=1,NMAX
          key%list%k(i)=zero
          key%list%ks(i)=zero
       enddo
       do i = 1, 2
          fieldk(i) = zero
       enddo
       if (n_ferr .gt. 0) call dcopy(f_errors, fieldk, min(2, n_ferr))
       if (l .eq. zero)  then
          div = one
       else
          div = l
       endif
       if(code.eq.14) then
          key%magnet="hkicker"
          key%list%k(1)=(node_value('kick ')+node_value('chkick ')+fieldk(1)/div)
       else if(code.eq.15) then
          key%magnet="kicker"
          key%list%k(1)=(node_value('hkick ')+node_value('chkick ')+fieldk(1)/div)
          key%list%ks(1)=(node_value('vkick ')+node_value('cvkick ')+fieldk(2)/div)
       else if(code.eq.16) then
          key%magnet="vkicker"
          key%list%ks(1)=(node_value('kick ')+node_value('cvkick ')+fieldk(2)/div)
       else
          key%magnet="marker"
       endif
       key%tiltd=node_value('tilt ')
    case(17)
       key%magnet="hmonitor"
    case(18)
       key%magnet="monitor"
    case(19)
       key%magnet="vmonitor"
       !  case(20)
       !     key%magnet="ecollimator"
       !     key%list%x_col=node_value('xsize ')
       !     key%list%y_col=node_value('ysize ')
       !     key%tiltd=node_value('tilt ')
       !  case(21)
       !     key%magnet="rcollimator"
       !     key%list%x_col=node_value('xsize ')
       !     key%list%y_col=node_value('ysize ')
       !     key%tiltd=node_value('tilt ')
    case(33)
       !---- This is the dipedge element
       edge= node_value('e1 ')
       hgap= node_value('hgap ')
       rhoi= bvk * node_value('h ')
       fint= node_value('fint ')
       corr= 2 * rhoi * hgap * fint
       if(rhoi .ne. zero .and. ( edge .ne. zero .or. corr .ne. zero )) then
          key%magnet="multipole"
          tanedg = tan(edge)
          secedg = one / cos(edge)
          psip = edge - corr * secedg * (one + sin(edge)**2)
          key%list%hf= rhoi * tanedg
          key%list%vf= -rhoi * tan(psip)
       else
          key%magnet="marker"
       endif
    case(24)
       key%magnet="instrument"
       key%tiltd=node_value('tilt ')
    case(27)
       key%magnet="twcavity"
       key%list%volt=bvk*node_value('volt ')
       freq=c_1d6*node_value('freq ')
       key%list%lag=node_value('lag ')*twopi
       offset_deltap=get_value('ptc_create_layout ','offset_deltap ')
       default=default+totalpath0 !fringe field calculation vitally relies on it!!!!
       if(offset_deltap.ne.zero) then
          freq=freq*((gammatr2-gamma2)*offset_deltap/gammatr2/gamma2+one)
       endif
       key%list%freq0=freq
       key%list%dphas=node_value("delta_lag ")
       key%list%psi=node_value("psi ")
       key%list%harmon=one
       if(key%list%volt.ne.zero.and.key%list%freq0.ne.zero) icav=1
    case(35)
       key%magnet="CHANGEREF"
       call dzero(patch_ang,3)
       call dzero(patch_trans,3)
       call get_node_vector('patch_ang ',3,patch_ang)
       call get_node_vector('patch_trans ',3,patch_trans)
       key%list%patchg=2
       do i=1,3
          key%list%ang(i)=patch_ang(i)
          key%list%t(i)=patch_trans(i)
       enddo
    case(37)
       key%magnet="rfcavity"
       key%list%volt=zero
       do i=1,NMAX
          key%list%k(i)=zero
          key%list%ks(i)=zero
       enddo
       key%list%k(1)=node_value('volt ')*c_1d_3
       ! vertical crab
       ! maybe requires a flip of sign
       !       key%list%ks(1)= (+/-)  node_value('volt ')*c_1d_3
       !
       freq=c_1d6*node_value('freq ')
       key%list%lag=node_value('lag ')*twopi+pih
       offset_deltap=get_value('ptc_create_layout ','offset_deltap ')
       if(offset_deltap.ne.zero) then
          default = getintstate()
          default=default+totalpath0
          call setintstate(default)
          freq=freq*((gammatr2-gamma2)*offset_deltap/gammatr2/gamma2+one)
       endif
       key%list%freq0=freq
       key%list%n_bessel=0
       key%list%harmon=one

       if(key%list%k(1).ne.zero.and.key%list%freq0.ne.zero) icav=1
    case default
       print*,"Element: ",name," not implemented in PTC"
       stop
    end select
100 continue
    if(code.ne.14.and.code.ne.15.and.code.ne.16) then
       do i=1,NMAX
          key%list%k(i)=bvk*key%list%k(i)
          key%list%ks(i)=bvk*key%list%ks(i)
       enddo
    endif
    call create_fibre(my_ring%end,key,EXCEPTION)

    if(advance_node().ne.0)  goto 10

    if (getdebug() > 0) then
       print*,' Length of machine: ',l_machine
    endif

    CALL GET_ENERGY(ENERGY,kin,BRHO,beta0,P0C)

    isclosedlayout=get_value('ptc_create_layout ','closed_layout ') .ne. 0

    if (getdebug() > 0) then
       if ( isclosedlayout .eqv. .true. ) then
          print *,'The machine is a RING'
       else
          print *,'The machine is a LINE'
       endif
    endif

    MY_RING%closed=isclosedlayout

    doneit=.true.
    call ring_l(my_ring,doneit)

    resplit=get_value('ptc_create_layout ','resplit ').ne.0
    if(resplit) then
       my_thin = get_value('ptc_create_layout ','thin ')
       my_xbend = get_value('ptc_create_layout ','xbend ')
       even = get_value('ptc_create_layout ','even ').ne.0
       resplit_cutting=2
       CALL THIN_LENS_resplit(my_ring,THIN=my_thin,even=even,xbend=my_xbend)
    endif

    if (getdebug() > 0) then
       write(6,*) "------------------------------------ PTC Survey ------------------------------------"
       write(6,*) "Before start: ",my_ring%start%chart%f%a
       write(6,*) "Before   end: ",my_ring%end%chart%f%b
    endif

    call survey(my_ring)

    if (getdebug() > 0) then
       write(6,*) "After  start: ",my_ring%start%chart%f%a
       write(6,*) "After    end: ",my_ring%end%chart%f%b
    endif

    call setintstate(default)

    if(my_ring%HARMONIC_NUMBER>0) then
       call get_length(my_ring,l)
       p=>my_ring%start
       do i=1,my_ring%n
          if(p%mag%kind==kind4) then
             if(p%mag%freq==zero) then
                write(6,*) " Bullshitting in MADX with Cavities ",my_ring%HARMONIC_NUMBER
                p%mag%freq=clight*my_ring%HARMONIC_NUMBER*BETA0/l
                p%magp%freq=p%mag%freq
             endif
          endif
          p=>p%next
       enddo
    endif

    if (getdebug() > 1) then
       print *, '^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^'
       print *, '^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^'
       print *, '^^^^^^    F I N I S H E D      P T C     I N P U T    ^^^^^^^^'
       print *, '^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^'
       print *, '^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^'
    endif

    return

  END subroutine ptc_input
  !_________________________________________________________________

  SUBROUTINE SUMM_MULTIPOLES_AND_ERRORS (l, key, normal_0123, skew_0123,ord_max)
    use twtrrfi ! integer, maxmul,maxferr,maxnaper
    implicit none
    ! 1) read multipole coeff. and errors for a current thick element
    ! 2) fill the error and multiploes arrays of data-bases
    REAL(dp), INTENT(IN) :: l
    type(keywords), INTENT(INOUT) ::  key
    REAL(dp), INTENT(OUT) :: normal_0123(0:3), skew_0123(0:3) ! n/l;
    REAL(dp) :: normal(0:maxmul), skew  (0:maxmul), &
         f_errors(0:maxferr), field(2,0:maxmul)
    INTEGER :: n_norm, n_skew, n_ferr ! number of terms in command line
    INTEGER :: node_fd_errors ! function
    integer :: i, i_count, n_dim_mult_err, ord_max

    !initialization
    normal_0123(:)=zero
    skew_0123(:)=zero
    do i=1,NMAX
       key%list%k(i)=zero
       key%list%ks(i)=zero
    enddo

    ! real(dp) f_errors(0:maxferr),normal(0:maxmul),skew(0:maxmul)
    ! Get multipole components on bench !-----------------------!
    call dzero(normal,maxmul+1) ! make zero "normal"            !
    call dzero(skew,maxmul+1)   ! make zero "skew"              !
    !                                                           !
    ! madxdict.h: "knl = [r, {0}], "                            !
    !             "ksl = [r, {0}], "                            !
    ! Assign values from the command line                       !
    call get_node_vector('knl ',n_norm,normal)                  !
    call get_node_vector('ksl ',n_skew,skew)                    !
    skew(0)=-skew(0)                                            ! frs error found 30.08.2008
    if(n_norm.ge.maxmul) n_norm=maxmul-1                        !
    if(n_skew.ge.maxmul) n_skew=maxmul-1                        !
    ord_max=max(n_norm,n_skew)                                  !
    ! void get_node_vector(char*par,int*length,double* vector)  !
    ! /* returns vector for parameter par of current element */ !
    !                                                           !
    ! get errors                                                !
    call dzero(f_errors,maxferr+1)                              !
    n_ferr = node_fd_errors(f_errors) !                         !
    ! /* returns the field errors of a node */                  !
    call dzero(field,2*(maxmul+1)) ! array to be zeroed.        !
    if (n_ferr .gt. 0) then                                     !
       call dcopy(f_errors,field,n_ferr)                        !
       ! subroutine dcopy(in,out,n)                             !
       ! Purpose:   Copy arrays.                                !
    endif                                                       !
    !-----------------------------------------------------------!

    ! fill strength of ALL normal multipoles
    if(n_norm.gt.0) then  ! ===============================!
       do i_count=0,n_norm                                 !
          if(i_count.gt.0) then                            !
             if(l.ne.zero) then                            !
                key%list%k(i_count+1)=normal(i_count)/l    !
             else                                          !
                key%list%k(i_count+1)=normal(i_count)      !
             endif                                         !
          endif                                            !
          if (i_count.le.3) then                           !
             if(l.ne.zero) then                            !
                normal_0123(i_count)=normal(i_count)/l     !
             else                                          !
                normal_0123(i_count)=normal(i_count)       !
             endif                                         !
          endif                                            !
       enddo                                               !
    endif !================================================!

    ! fill strength of ALL skew multipoles
    if(n_skew.gt.0) then  ! ===============================!
       do i_count=0,n_skew                                 !
          if(i_count.gt.0) then                            !
             if(l.ne.zero) then                            !
                key%list%ks(i_count+1)=skew(i_count)/l     !
             else                                          !
                key%list%ks(i_count+1)=skew(i_count)       !
             endif                                         !
          endif                                            !
          if (i_count.le.3) then                           !
             if(l.ne.zero) then                            !
                skew_0123(i_count)=skew(i_count)/l         !
             else                                          !
                skew_0123(i_count)=skew(i_count)           !
             endif                                         !
          endif                                            !
       enddo                                               !
    endif !================================================!

    n_dim_mult_err = max(n_norm, n_skew, n_ferr/2) !===========!
    if(n_dim_mult_err.ge.maxmul) n_dim_mult_err=maxmul-1       !
    if(n_ferr.gt.0) then                                       !
       do i_count=0,n_dim_mult_err                             !
          if(l.ne.zero) then                                   !
             key%list%k(i_count+1)=key%list%k(i_count+1)+ &    !
                  field(1,i_count)/l                           !
             key%list%ks(i_count+1)=key%list%ks(i_count+1)+ &  !
                  field(2,i_count)/l                           !
          else                                                 !
             key%list%k(i_count+1)=key%list%k(i_count+1)+ &    !
                  field(1,i_count)                             !
             key%list%ks(i_count+1)=key%list%ks(i_count+1)+ &  !
                  field(2,i_count)                             !
          endif                                                !
       enddo                                                   !
    endif !====================================================!



  END SUBROUTINE SUMM_MULTIPOLES_AND_ERRORS
  !----------------------------------------------------------------

  subroutine ptc_getnfieldcomp(fibreidx, ncomp, nval)
    implicit none
    real(kind(1d0))      :: nval
    integer              :: fibreidx
    integer              :: ncomp
    type(fibre), pointer :: p
    integer              :: j

    p=>my_ring%start
    do j=1, fibreidx
       p=>p%next
    enddo

    ncomp = ncomp + 1
    nval = p%mag%BN(ncomp)

  end subroutine  ptc_getnfieldcomp
  !----------------------------------------------------------------

  subroutine ptc_getsfieldcomp(fibreidx, ncomp, nval)
    implicit none
    real(kind(1d0))      :: nval
    integer              :: fibreidx
    integer              :: ncomp
    type(fibre), pointer :: p
    integer              :: j

    p=>my_ring%start
    do j=1, fibreidx
       p=>p%next
    enddo

    ncomp = ncomp + 1

    nval = p%mag%AN(ncomp)
    print*, "Returning AN",nval," for ",p%mag%name


  end subroutine  ptc_getsfieldcomp
  !----------------------------------------------------------------

  subroutine ptc_setfieldcomp(fibreidx)
    implicit none
    integer              :: fibreidx
    type(fibre), pointer :: p
    integer              :: j, i
    integer              :: kn, ks
    real(dp)             :: v
    real(kind(1d0)) get_value

    if ( .not. associated(my_ring) ) then
       call fort_warn("ptc_setfieldcomp","No active PTC layout/period")
       return
    endif

    if (getdebug()>2) then
       print*, "I am in ptc_setfieldcomp: Element index is ", fibreidx
    endif

    if ( (fibreidx .lt. 1) .and. (fibreidx .gt. my_ring%n) ) then
       call fort_warn("ptc_setfieldcomp","element out of range of the current layout")
       return
    endif

    p=>my_ring%start
    do j=1, fibreidx
       p=>p%next
    enddo

    if (getdebug() > 1 ) then
       print*,"Found element no. ", fibreidx," named ", p%mag%name, &
            &" of kind ", p%mag%kind, mytype(p%mag%kind)
       print*,"Currently nmul is ", p%mag%p%nmul

       write(6,*) "BNs",p%mag%BN
       write(6,*) "ANs",p%mag%AN

       DO i=1,p%mag%p%nmul
          print*, "Polimorphic BN(",i,")"
          call print(p%mag%BN(i),6)
          print*, "Polimorphic AN(",i,")"
          call print(p%mag%AN(i),6)
       ENDDO

    endif

    kn = get_value('ptc_setfieldcomp ','kn ')
    v = get_value('ptc_setfieldcomp ','value ')

    if (kn >= 0) then
       kn = kn + 1

       if (getdebug() > 1) print*,"Setting up KN ", kn, " from ", p%mag%BN(kn) ," to ", v

       call add(p%mag, kn,0,v)
       call add(p%magp,kn,0,v)


    else
       ks = get_value('ptc_setfieldcomp ','ks ')
       if (ks < 0) then
          call fort_warn("ptc_setfieldcomp","neither kn nor ks specified")
          return
       endif
       ks = ks + 1

       !      print*,"Setting up skew field component ", ks," to ", v

       if (getdebug() > 1) print*,"Setting up KS ", ks, " from ", p%mag%AN(ks) ," to ", v
       call add(p%mag, -ks,0,v)
       call add(p%magp,-ks,0,v)

    endif

    if (getdebug() > 1 ) then
       write(6,*) "BNs",p%mag%BN
       write(6,*) "ANs",p%mag%AN
       write(6,*) ""
    endif
  end subroutine ptc_setfieldcomp
  !----------------------------------------------------------------

  subroutine ptc_align()
    use twiss0fi
    implicit none
    integer j,n_align,node_al_errors
    integer restart_sequ,advance_node
    real(dp) al_errors(align_max)
    type(fibre), pointer :: f
    !---------------------------------------------------------------


    j=restart_sequ()
    j=0
    f=>my_ring%start
10  continue

    j=j+1
    n_align = node_al_errors(al_errors)
    if (n_align.ne.0)  then
       !       write(6,'(6f8.3)')  al_errors(1:6)
       call mad_misalign_fibre(f,al_errors(1:6))
    endif
    f=>f%next
    if(advance_node().ne.0)  goto 10

  END subroutine ptc_align
  !_________________________________________________________________

  subroutine ptc_dumpmaps()
    !Dumps to file maps and/or matrixes (i.e. first order maps)
    implicit none
    type(fibre), pointer :: p
    type(damap)          :: id !identity map used for calculating maps for each element
    type(real_8)         :: y2(6)  !polimorphes array used for calculating maps for each element
    type(real_8)         :: yfull(6)  !polimorphes array used for calculating maps for each element
    real(dp)             :: xt(6)
    integer              :: i !iterators
    integer mf1,mf2
    character(200)       :: filename='ptcmaps.txt'
    character(200)       :: filenamefull='ptcmaps'
    integer              :: flag_index,why(9)
    character(200)       :: whymsg
    real(kind(1d0))      :: suml=zero
    integer  geterrorflag !C function that returns errorflag value

    suml=zero

    if (cavsareset .eqv. .false.) then
       call setcavities(my_ring,maxaccel)
       if (geterrorflag() /= 0) then
          return
       endif
    endif

    if (getdebug() > 1) print *, '<madx_ptc_module.f90 : ptc_dumpmaps> Maps are dumped to file ',filename
    call kanalnummer(mf1)
    open(unit=mf1,file=filename)

    !    write(filenamefull,*) filename,".",my_ring%start%mag%name,"-",my_ring%end%mag%name,".txt"
    filenamefull="ptcmaps.start-end.txt"
    print*, filenamefull
    call kanalnummer(mf2)
    open(unit=mf2,file=filenamefull)

    print*, "no=1"," mynd2=",c_%nd2," npara=",c_%npara
    call init(getintstate(),1,c_%np_pol,berz)

    call alloc(id);
    call alloc(y2);
    call alloc(yfull);

    xt(:) = zero
    id    = 1     ! making identity map

    yfull  = xt + id

    p=>my_ring%start
    do i=1,my_ring%n


       y2=xt+id ! we track identity map from the current position

       if( (p%mag%kind/=kind21) .and. (p%mag%kind/=kind4) ) then

          call track(my_ring,y2,i,i+1,getintstate())

          if (( .not. check_stable ) .or. ( .not. c_%stable_da )) then
             call fort_warn('ptc_dumpmaps: ','DA got unstable')
             call seterrorflag(10,"ptc_dumpmaps ","DA got unstable ");
             close(mf1)
             close(mf2)
             return
          endif

          call PRODUCE_APERTURE_FLAG(flag_index)
          if(flag_index/=0) then
             call ANALYSE_APERTURE_FLAG(flag_index,why)

             Write(6,*) "ptc_dumpmaps: APERTURE error for element: ",i," name: ",p%MAG%name
             write(whymsg,*) 'APERTURE error: ',why
             call fort_warn('ptc_dumpmaps: ',whymsg)
             call seterrorflag(10,"ptc_dumpmaps: ",whymsg);
             c_%watch_user=.false.
             close(mf1)
             close(mf2)
             return
          endif

          call track(my_ring,xt,i,i+1,getintstate())
          if (( .not. check_stable ) .or. ( .not. c_%stable_da )) then
             call fort_warn('ptc_dumpmaps: ','DA got unstable')
             call seterrorflag(10,"ptc_dumpmaps ","DA got unstable ");
             close(mf1)
             close(mf2)
             return
          endif

          call PRODUCE_APERTURE_FLAG(flag_index)
          if(flag_index/=0) then
             call ANALYSE_APERTURE_FLAG(flag_index,why)
             Write(6,*) "ptc_dumpmaps: APERTURE error for element: ",i," name: ",p%MAG%name
             write(whymsg,*) 'APERTURE error: ',why
             call fort_warn('ptc_dumpmaps: ',whymsg)
             call seterrorflag(10,"ptc_dumpmaps: ",whymsg);
             c_%watch_user=.false.
             close(mf1)
             close(mf2)
             return
          endif
       else
          if (getdebug() > 2) print *, 'Track Cavity...'

          call track(my_ring,y2,i,i+2,getintstate())
          if (( .not. check_stable ) .or. ( .not. c_%stable_da )) then
             call fort_warn('ptc_dumpmaps: ','DA got unstable')
             call seterrorflag(10,"ptc_dumpmaps ","DA got unstable ");
             close(mf1)
             close(mf2)
             return
          endif

          call PRODUCE_APERTURE_FLAG(flag_index)
          if(flag_index/=0) then
             call ANALYSE_APERTURE_FLAG(flag_index,why)
             Write(6,*) "ptc_dumpmaps: APERTURE error for element: ",i," name: ",p%MAG%name
             write(whymsg,*) 'APERTURE error: ',why
             call fort_warn('ptc_dumpmaps: ',whymsg)
             call seterrorflag(10,"ptc_dumpmaps: ",whymsg);
             c_%watch_user=.false.
             close(mf1)
             close(mf2)
             return
          endif

          call track(my_ring,xt,i,i+2,getintstate())
          if (( .not. check_stable ) .or. ( .not. c_%stable_da )) then
             call fort_warn('ptc_dumpmaps: ','DA got unstable')
             call seterrorflag(10,"ptc_dumpmaps ","DA got unstable ");
             close(mf1)
             close(mf2)
             return
          endif

          call PRODUCE_APERTURE_FLAG(flag_index)
          if(flag_index/=0) then
             call ANALYSE_APERTURE_FLAG(flag_index,why)
             Write(6,*) "ptc_dumpmaps: APERTURE error for element: ",i," name: ",p%MAG%name
             write(whymsg,*) 'APERTURE error: ',why
             call fort_warn('ptc_dumpmaps: ',whymsg)
             call seterrorflag(10,"ptc_dumpmaps: ",whymsg);
             c_%watch_user=.false.
             close(mf1)
             close(mf2)
             return
          endif
       endif


       call track(my_ring,yfull,i,i+1,getintstate())

       if (( .not. check_stable ) .or. ( .not. c_%stable_da )) then
          call fort_warn('ptc_dumpmaps: ','DA got unstable')
          call seterrorflag(10,"ptc_dumpmaps ","DA got unstable ");
          close(mf1)
          close(mf2)
          return
       endif

       call PRODUCE_APERTURE_FLAG(flag_index)
       if(flag_index/=0) then
          call ANALYSE_APERTURE_FLAG(flag_index,why)

          Write(6,*) "ptc_dumpmaps: APERTURE error for element: ",i," name: ",p%MAG%name
          write(whymsg,*) 'APERTURE error: ',why
          call fort_warn('ptc_dumpmaps: ',whymsg)
          call seterrorflag(10,"ptc_dumpmaps: ",whymsg);
          c_%watch_user=.false.
          close(mf1)
          close(mf2)
          return
       endif

       write(mf2,*) p%mag%name, suml,' m ==========================='
       call print(yfull,mf2)

       suml=suml+p%MAG%P%ld

       write(mf1,*) p%mag%name, suml,' m ==========================='
       if (c_%npara == 6) then
          call dump6dmap(y2, mf1)
       elseif (c_%npara == 5) then
          call dump5dmap(y2, mf1)
       elseif (c_%npara == 4) then
          call dump4dmap(y2, mf1)
       else
          call fort_warn("ptc_dumpmaps","c_%npara is neither 6,5 nor 4")
       endif
       p=>p%next
    enddo

    close(mf1)
    call kill(y2);
    call kill(id);

    !_________________________________________________________________
    !_________________________________________________________________
    !_________________________________________________________________

  contains
    !_________________________________________________________________
    subroutine dump4dmap(y2, fun)
      implicit none
      double precision a1000,a0100,a0010,a0001
      type(real_8) :: y2(6)  !polimorphes array used for calculating maps for each element
      integer      :: fun !file unit number
      integer      :: ii

      if (getdebug() > 1) then

      endif

      do ii=1,4
         a1000=y2(ii).sub.'1000'
         a0100=y2(ii).sub.'0100'
         a0010=y2(ii).sub.'0010'
         a0001=y2(ii).sub.'0001'
         write(fun,'(6f13.8)')  a1000, &
              &                 a0100, &
              &                 a0010, &
              &                 a0001
      enddo

    end subroutine dump4dmap
    !_________________________________________________________________

    subroutine dump5dmap(y2, fun)
      implicit none
      double precision a10000,a01000,a00100,a00010,a00001
      type(real_8) :: y2(6)  !polimorphes array used for calculating maps for each element
      integer      :: fun !file unit number
      integer      :: ii
      do ii=1,5
         a10000=y2(ii).sub.'10000'
         a01000=y2(ii).sub.'01000'
         a00100=y2(ii).sub.'00100'
         a00010=y2(ii).sub.'00010'
         a00001=y2(ii).sub.'00001'
         write(fun,'(6f13.8)')  a10000, &
              &                 a01000, &
              &                 a00100, &
              &                 a00010, &
              &                 a00001     !
      enddo

    end subroutine dump5dmap
    !_________________________________________________________________

    subroutine dump6dmap(y2, fun)
      implicit none
      double precision a100000,a010000,a001000,a000100,a000010,a000001
      type(real_8) :: y2(6)  !polimorphes array used for calculating maps for each element
      integer      :: fun !file unit number
      integer      :: ii

      do ii=1,4
         a100000=y2(ii).sub.'100000'
         a010000=y2(ii).sub.'010000'
         a001000=y2(ii).sub.'001000'
         a000100=y2(ii).sub.'000100'
         a000010=y2(ii).sub.'000010'
         a000001=y2(ii).sub.'000001'
         write(fun,'(6f13.8)')  a100000, &
              &                 a010000, &
              &                 a001000, &
              &                 a000100, &
              &                 a000001, & !madx format has dp/p at the last column
              &                 a000010    !
      enddo

      do ii=6,5,-1
         a100000=y2(ii).sub.'100000'
         a010000=y2(ii).sub.'010000'
         a001000=y2(ii).sub.'001000'
         a000100=y2(ii).sub.'000100'
         a000010=y2(ii).sub.'000010'
         a000001=y2(ii).sub.'000001'
         write(fun,'(6f13.8)')  a100000, &
              &                 a010000, &
              &                 a001000, &
              &                 a000100, &
              &                 a000001, & !madx format has dp/p at the last column
              &                 a000010    !
      enddo
    end subroutine dump6dmap


  end subroutine ptc_dumpmaps

  RECURSIVE FUNCTION FACTORIAL (N) &
       RESULT (FACTORIAL_RESULT)
    INTEGER :: N, FACTORIAL_RESULT

    IF (N <= 0 ) THEN
       FACTORIAL_RESULT = 1
    ELSE
       FACTORIAL_RESULT = N * FACTORIAL (N-1)
    END IF
  END FUNCTION FACTORIAL

  subroutine ptc_track()
    implicit none
    integer i,nint,ndble,nchar,int_arr(1),char_l,icase,turns,flag_index,why(9)
    integer j,next_start
    real(dp) x0(6),x(6),deltap0,deltap,dt
    real(dp)  xx,pxx,yx,pyx,tx,deltaex,fxx,phixx,fyx,phiyx,ftx,phitx
    real(kind(1d0)) get_value
    logical(lp) closed_orbit
    character*12 char_a
    data char_a / ' ' /
    !------------------------------------------------------------------------------


    if(universe.le.0) then
       call fort_warn('return from ptc_track: ',' no universe created')
       return
    endif
    if(index_mad.le.0) then
       call fort_warn('return from ptc_track: ',' no layout created')
       return
    endif

    icase = get_value('ptc_track ','icase ')
    deltap0 = get_value('ptc_track ','deltap ')

    deltap = zero
    call my_state(icase,deltap,deltap0)

    if (getdebug() > 2) then
       print *, "ptc_track: internal state is:"
       call print(default,6)
    endif

    x0(:)=zero
    if(mytime) then
       call Convert_dp_to_dt (deltap, dt)
    else
       dt=deltap
    endif
    if(icase.eq.5) x0(5)=dt
    closed_orbit = get_value('ptc_track ','closed_orbit ') .ne. 0
    if(closed_orbit) then
       call find_orbit(my_ring,x0,1,default,c_1d_7)
       CALL write_closed_orbit(icase,x0)
    endif


    call comm_para('coord ',nint,ndble,nchar,int_arr,x,char_a,char_l)

    j  =  next_start(xx,pxx,yx,pyx,tx,deltaex,fxx,phixx,fyx,phiyx,ftx,phitx)
    print*,"dat1",j,xx,pxx,yx,pyx,tx,deltaex,fxx,phixx,fyx,phiyx,ftx,phitx
    j  =  next_start(xx,pxx,yx,pyx,tx,deltaex,fxx,phixx,fyx,phiyx,ftx,phitx)
    print*,"dat2",j,xx,pxx,yx,pyx,tx,deltaex,fxx,phixx,fyx,phiyx,ftx,phitx
    j  =  next_start(xx,pxx,yx,pyx,tx,deltaex,fxx,phixx,fyx,phiyx,ftx,phitx)
    print*,"dat3",j,xx,pxx,yx,pyx,tx,deltaex,fxx,phixx,fyx,phiyx,ftx,phitx

    x(:)=x(:)+x0(:)
    print*,"  Initial Coordinates: ", x
    turns = get_value('ptc_track ','turns ')
    c_%watch_user=.true.
    do i=1,turns
       call track(my_ring,x,1,default)
       if (( .not. check_stable ) .or. ( .not. c_%stable_da )) then
          call fort_warn('ptc_track: ','DA got unstable')
          call seterrorflag(10,"ptc_track ","DA got unstable ");
          return
       endif
       call PRODUCE_APERTURE_FLAG(flag_index)
       if(flag_index/=0) then
          call ANALYSE_APERTURE_FLAG(flag_index,why)
          Write(6,*) "ptc_track unstable (tracking)-programs continues "
          Write(6,*) why ! See produce aperture flag routine in sd_frame
          goto 100
       endif
    enddo
    c_%watch_user=.false.
    print*,"  End Coordinates: ",x
    return
100 continue
    c_%watch_user=.false.
    print*,"  Last Coordinates: ",x," after: ",i," turn(s)"

  END subroutine ptc_track



  !________________________________________________________________________________


  subroutine ptc_end()
    implicit none
    integer i

    if(universe.le.0) then
       call fort_warn('return from ptc_end: ',' no universe can be killed')
       return
    endif

    call killsavedmaps() !module ptc_twiss -> kill buffered maps

    !    call killparresult()
    call resetknobs()  !remove the knobs

    if ( associated(m_u%n) .eqv. .false. ) then
       print*, "We attempt to kill not initialized universe!"
    endif


    call kill_universe(m_u)
    nullify(my_ring)
    call kill_tpsa
    do i=1,size(s_b)
       call nul_coef(s_b(i))
    enddo
    deallocate(s_b)
    firsttime_coef=.true.

    universe=universe-1
  end subroutine ptc_end


  subroutine normalform_normalform(s1,s2)
    implicit none
    type (normalform),intent(inout)::s1
    type (normalform),intent(in)::s2
    integer i,j

    s1%a_t=s2%a_t
    s1%a1=s2%a1
    s1%a%constant(:)=s2%a%constant(:)
    s1%a%Linear=s2%a%Linear
    s1%a%nonlinear=s2%a%nonlinear
    s1%a%pb=s2%a%pb
    s1%normal%constant(:)=s2%normal%constant(:)
    s1%normal%Linear=s2%normal%Linear
    s1%normal%nonlinear=s2%normal%nonlinear
    s1%normal%pb=s2%normal%pb
    s1%DHDJ=s2%DHDJ
    do i=1,ndim
       s1%TUNE(i)=s2%TUNE(i)
       s1%damping(i)=s2%damping(i)
       s1%plane(i)=s2%plane(i)
       do j=1,mynreso
          s1%m(i,j)=s2%m(i,j)
       enddo
    enddo
    s1%nord=s2%nord
    s1%jtune=s2%jtune
    s1%nres=s2%nres
    s1%AUTO=s2%AUTO
  end subroutine normalform_normalform
  !_________________________________________________________________


  SUBROUTINE set_PARAMETERS(R,nt,iorder,IFAM,inda,scale)
    !Strength of Multipole of order iorder as parameter

    IMPLICIT NONE
    integer ipause, mypause
    logical(lp) ok
    INTEGER iorder,i,j,jj,k,lstr,IFAM,tot,nt,inda,min1
    INTEGER,parameter::ipara=100
    real(dp) scale(ipara),value
    character(20) str
    CHARACTER(3) STR1
    character(10),dimension(10)::multname
    type(layout) r
    type(fibre), POINTER :: current
    INTEGER,ALLOCATABLE,dimension(:)::DAFAM
    INTEGER,ALLOCATABLE,dimension(:,:)::FAM
    real(dp),ALLOCATABLE,dimension(:)::SFAM
    multname=(/"Dipole    ","Quadrupole","Sextupole ","Octupole  ","Decapole  ",&
         "Dodecapole","14-Pole   ","16-Pole   ","18-Pole   ","20-Pole   "/)

    ALLOCATE(FAM(IFAM,0:R%N),DAFAM(IFAM),SFAM(IFAM))

    min1=0
    if(iorder.lt.0) then
       min1=1
       iorder=-iorder
    endif
    DO I=1,IFAM
       OK=.TRUE.
       DO WHILE(OK)
          TOT=0
          if(min1.eq.0) WRITE(6,*) " Identify ",multname(iorder)
          if(min1.eq.1) WRITE(6,*) " Identify ","SKEW-"//multname(iorder)
          READ(5,*) STR
          STR=TRIM(ADJUSTL(STR))
          LSTR=LEN_TRIM (STR)
          current=>r%start
          DO J=1,R%N
             IF(current%MAG%NAME==STR.and.current%MAG%P%NMUL==iorder) THEN
                TOT=TOT+1
                FAM(I,TOT)=J
             ENDIF
             current=>current%next
          ENDDO
          WRITE(6,*) TOT," Is that OK? YES or NO?"
          READ (5,*) STR1
          STR1=TRIM(ADJUSTL(STR1))
          IF(STR1(1:1)=='Y'.OR.STR1(1:1)=='y') THEN
             OK=.FALSE.
             inda=inda+1
             if(inda.gt.100) then
                write(6,*) " Problem: Only ",ipara," Parameters allowed"
                ipause=mypause(2002)
             endif
             DAFAM(I)=inda
             WRITE(6,*) " Give Scaling Factor, '0' uses Default"
             read(5,*) value
             if(value==0) then
                WRITE(6,*) " Take Default Scaling Value : ",scale(inda)
                SFAM(I)=scale(inda)
             else
                SFAM(I)=value
             endif
          ENDIF
       ENDDO

       FAM(I,0)=TOT
       current=r%start
       DO JJ=1,FAM(I,0)
          J=FAM(I,JJ)
          ! ALLOCATION GYMNASTIC IF Multipole NOT YET ALLOCATED
          IF(current%MAGP%P%NMUL<iorder) THEN
             CALL KILL(current%MAGP%BN,current%MAGP%P%NMUL)
             CALL KILL(current%MAGP%AN,current%MAGP%P%NMUL)
             current%MAGP%P%NMUL=iorder
             DEALLOCATE(current%MAGP%BN)
             DEALLOCATE(current%MAGP%AN)
             CALL ALLOC(current%MAGP%BN,iorder)
             CALL ALLOC(current%MAGP%AN,iorder)
             ALLOCATE(current%MAGP%BN(iorder),current%MAGP%AN(iorder))
             DO K=1,current%MAG%P%NMUL
                current%MAGP%BN(K)=current%MAG%BN(K)
                current%MAGP%AN(K)=current%MAG%AN(K)
             ENDDO
             DEALLOCATE(current%MAG%BN)
             DEALLOCATE(current%MAG%AN)
             ALLOCATE(current%MAG%BN(iorder),current%MAG%AN(iorder))
             call equal(current%MAG,current%MAGP)
          ENDIF
          if(min1.eq.0) then
             current%MAGP%BN(iorder)%I=NT+I
             current%MAGP%BN(iorder)%KIND=3
          else
             current%MAGP%AN(iorder)%I=NT+I
             current%MAGP%AN(iorder)%KIND=3
          endif
          current=>current%next
       ENDDO
    ENDDO

    current=r%start
    DO I=1,IFAM
       DO JJ=1,1
          J=FAM(I,JJ)
          if(min1.eq.0) WRITE(6,*)  current%MAG%NAME,' ', current%MAG%BN(iorder)
          if(min1.eq.1) WRITE(6,*)  current%MAG%NAME,' ', current%MAG%AN(iorder)
          current=>current%next
       ENDDO
    ENDDO

    DEALLOCATE(FAM,STAT=I)
    !    WRITE(6,*) I
    DEALLOCATE(DAFAM,STAT=I)
    !    WRITE(6,*) I
    DEALLOCATE(SFAM,STAT=I)
    !    WRITE(6,*) I

  end subroutine set_PARAMETERS
  !______________________________________________________________________

  subroutine my_state(icase,deltap,deltap0)
    implicit none
    integer icase,i
    real(dp) deltap0,deltap

    default = getintstate()

    if (getdebug()>1) then
       print*, "icase=",icase," deltap=",deltap," deltap0=",deltap0
    endif

    deltap = zero
    select case(icase)
    CASE(4)
       if (getdebug()>1) print*, "my_state: Enforcing ONLY_4D+NOCAVITY and NO DELTA"
       default=default-delta0
       default=default+only_4d0+NOCAVITY0
       i=4
    CASE(5)
       if (getdebug()>1) print*, "my_state: Enforcing DELTA"
       default=default+delta0
       deltap=deltap0
       i=5
    CASE(56)
       if (getdebug()>1) print*, "my_state: Enforcing coasting beam"
       default = default - delta0 - only_4d0
       default = default + NOCAVITY0
       deltap=deltap0
       i=56
    CASE(6)
       i=6
    CASE DEFAULT
       default=default+only_4d0+NOCAVITY0
       i=4
    END SELECT

    if (i==6) then
       if ( (icav==0) .and. my_ring%closed .and. (getenforce6D() .eqv. .false.)) then
          default = default - delta0 - only_4d0
          default=default +  NOCAVITY0
          call fort_warn('return mystate: ',' no cavity - dimensionality reduced 6 -> 5 and 1/2')
          i=56
       else
          default = default - delta0 - only_4d0 - NOCAVITY0 !enforcing nocavity to false
       endif
    endif

    call setintstate(default)
    CALL UPDATE_STATES

    if (getdebug()>0) call print(default,6)

    icase = i

  end subroutine my_state

  !______________________________________________________________________

  subroutine f90flush(i,option)
    implicit none
    integer i,ios
    logical(lp) ostat, fexist,option
    logical fexist1, ostat1
    character*20 faction,faccess,fform,fwstat
    character*255 fname
    inquire(err=1,iostat=ios,&
         unit=i,opened=ostat1,exist=fexist1,write=fwstat)
    fexist = fexist1
    ostat  = ostat1
    if (.not.ostat.or..not.fexist.or.fwstat.ne.'YES') return
    inquire(err=2,iostat=ios,&
         unit=i,action=faction,access=faccess,&
         form=fform,name=fname)
    close (unit=i,err=3)
    !     write (*,*) 'Re-opening ',i,' ',faction,faccess,fform,fname
    if (option) then
       open(err=4,iostat=ios,&
            unit=i,action=faction,access=faccess,form=fform,&
            file=fname,status='old',position='append')
    else
       open(err=4,iostat=ios,&
            unit=i,action=faction,access=faccess,form=fform,&
            file=fname,status='old',position='rewind')
    endif
    return
1   write (*,*)&
         ' F90FLUSH 1st INQUIRE FAILED with IOSTAT ',ios,' on UNIT ',i
    stop
2   write (*,*)&
         ' F90FLUSH 2nd INQUIRE FAILED with IOSTAT ', ios,' on UNIT ',i
    stop
3   write (*,*)&
         ' F90FLUSH CLOSE FAILED with IOSTAT ',ios,' on UNIT ',i
    stop
4   write (*,*)&
         ' F90FLUSH RE-OPEN FAILED with IOSTAT ',ios,' on UNIT ',i
    stop
  end subroutine f90flush

  SUBROUTINE write_closed_orbit(icase,x)
    implicit none
    INTEGER,  INTENT(IN):: icase
    REAL (dp),INTENT(IN) :: x(6)
    if(icase.eq.4) then
       print*,"Closed orbit: ",x(1),x(2),x(3),x(4)
    elseif(icase.eq.5) then
       print*,"Closed orbit: ",x(1),x(2),x(3),x(4),x(5)
    elseif(icase.eq.6) then
       print*,"Closed orbit: ",x(1),x(2),x(3),x(4),-x(6),x(5)
    endif
  ENDSUBROUTINE write_closed_orbit

  SUBROUTINE Convert_dp_to_dt(deltap, dt)
    implicit none
    ! convert deltap=(p-p0)/p0 to dt=deltaE/p0c
    REAL(dp), INTENT(IN)  :: deltap
    REAL(dp), INTENT(OUT) :: dt

    ! local
    real(dp) :: MASS_GeV, ENERGY,KINETIC,BRHO,BETA0,P0C,gamma0I,gambet

    ! to get "energy" value
    Call GET_ONE(MASS_GeV,ENERGY,KINETIC,BRHO,BETA0,P0C,gamma0I,gambet)

    IF (beta0.gt.zero ) THEN
       dt=SQRT(deltap*(deltap+two)+one/beta0/beta0)-one/beta0
    ELSE  ! exculde devision by 0
       call aafail('SUBR. Convert_dp_to_dt: ',' CALL GET_ONE => beta0.LE.0')
    ENDIF

  END SUBROUTINE Convert_dp_to_dt
  !=============================================================================

  subroutine makemaptable(y)
    implicit none
    type(real_8):: y(6)
    integer,parameter :: i_map_coor=10
    integer           :: map_term, ja(6),i,ii,iii
    integer           :: i1,i2,i3,i4,i5,i6
    integer           :: order, no
    real(dp)          :: coef
    real(kind(1d0))   :: map_coor(i_map_coor)
    real(kind(1d0))   :: get_value
    !    type(universal_taylor) :: ut

    !    write(0,*) "MAP_TABLE"

    map_term=42
    call  make_map_table(map_term)

    order = get_value("ptc_normal ","no ")

    call liepeek(iia,icoast)
    allocate(j(c_%npara))
    ja(:)    = 0
    j(:)     = 0

    goto 100 ! skip the code that was in place until 29 March 2010

    do iii=1,c_%npara
       coef = y(iii)%T.sub.j
       ! following works
       !coef = y(iii)%T.sub.mapSelector5variables(1)
       map_coor(1)=coef
       map_coor(2)=iii
       map_coor(3)=c_%npara
       map_coor(4)=0
       map_coor(5)=ja(1)
       map_coor(6)=ja(2)
       map_coor(7)=ja(3)
       map_coor(8)=ja(4)
       map_coor(9)=ja(5)
       map_coor(10)=ja(6)
       call vector_to_table("ptc_normal ", 'coef ', i_map_coor, map_coor(1))
       call augment_count("ptc_normal ")
    enddo

    do i = 1,c_%npara

       do ii = 1,c_%npara
          j(ii) = 1
          ja(ii) = j(ii)
          coef = y(i)%T.sub.j
          map_coor(1)=coef
          map_coor(2)=i
          map_coor(3)=c_%npara! 29.06.2006 here was iia(2) - to be verified
          map_coor(4)=sum(ja(:))
          map_coor(5)=ja(1)
          map_coor(6)=ja(2)
          map_coor(7)=ja(3)
          map_coor(8)=ja(4)
          map_coor(9)=ja(5)
          map_coor(10)=ja(6)
          call vector_to_table("ptc_normal ", 'coef ', i_map_coor, map_coor(1))
          call augment_count("ptc_normal ")
          j(:)  = 0
          ja(ii) = j(ii)
       enddo

       !           ut = y(i)
       !           do ii = 1,ut%n
       !              map_coor(1)=ut%c(ii) !coef
       !              map_coor(2)=i !index of taylor
       !              map_coor(3)=c_%npara
       !              map_coor(4)=sum(ut%j(i,:)) !order
       !              map_coor(5)=ut%j(ii,1)
       !              map_coor(6)=ut%j(ii,2)
       !              map_coor(7)=ut%j(ii,3)
       !              map_coor(8)=ut%j(ii,4)
       !              map_coor(9)=ut%j(ii,5)
       !              map_coor(10)=ut%j(ii,6)
       !           enddo


    enddo

    ! note that the order in which the coefficients appear in the map_table slightly
    ! differ from the order in which they appear in fort.18
100 do i=1,c_%npara ! distribute exponents over 6 variables, knowing their sum
       do no=0,order
          if (c_%npara.eq.6) then
             do i1=no,0,-1
                do i2=no-i1,0,-1
                   do i3=no-i1-i2,0,-1
                      do i4=no-i1-i2-i3,0,-1
                         do i5=no-i1-i2-i3-i4,0,-1
                            do i6=no-i1-i2-i3-i4-i5,0,-1
                               if (i1+i2+i3+i4+i5+i6==no) then
                                  !write(0,'(6(i4))'), i1,i2,i3,i4,i5,i6
                                  j(1)=i1
                                  j(2)=i2
                                  j(3)=i3
                                  j(4)=i4
                                  j(5)=i5
                                  j(6)=i6
                                  coef = y(i)%T.sub.j
                                  if (coef.ne.zero) then
                                     map_coor(1)=coef
                                     map_coor(2)=i
                                     map_coor(3)=c_%npara
                                     map_coor(4)=no
                                     map_coor(5)=j(1)
                                     map_coor(6)=j(2)
                                     map_coor(7)=j(3)
                                     map_coor(8)=j(4)
                                     map_coor(9)=j(5)
                                     map_coor(10)=j(6)
                                     !call vector_to_table("map_table ", 'coef ', i_map_coor, map_coor(1))
                                     call augment_count("map_table ")
                                  endif
                                  !write(0,*) 'write coef', coef
                               endif
                            enddo
                         enddo
                      enddo
                   enddo
                enddo
             enddo
          elseif (c_%npara.eq.5) then ! distribute exponents over 5 variables, knowing their sum
             do i1=no,0,-1
                do i2=no-i1,0,-1
                   do i3=no-i1-i2,0,-1
                      do i4=no-i1-i2-i3,0,-1
                         do i5=no-i1-i2-i3-i4,0,-1
                            if (i1+i2+i3+i4+i5==no) then
                               j(1)=i1
                               j(2)=i2
                               j(3)=i3
                               j(4)=i4
                               j(5)=i5
                               coef = y(i)%T.sub.j
                               if (coef.ne.zero) then
                                  map_coor(1)=coef
                                  map_coor(2)=i
                                  map_coor(3)=c_%npara
                                  map_coor(4)=no
                                  map_coor(5)=j(1)
                                  map_coor(6)=j(2)
                                  map_coor(7)=j(3)
                                  map_coor(8)=j(4)
                                  map_coor(9)=j(5)
                                  map_coor(10) = 0
                                  call vector_to_table("map_table ", 'coef ', i_map_coor, map_coor(1))
                                  call augment_count("map_table ")
                               endif
                            endif
                         enddo
                      enddo
                   enddo
                enddo
             enddo
          elseif (c_%npara.eq.4) then ! distribute exponents over 4 variables, knowing their sum
             do i1=no,0,-1
                do i2=no-i1,0,-1
                   do i3=no-i1-i2,0,-1
                      do i4=no-i1-i2-i3,0,-1
                         if (i1+i2+i3+i4==no) then
                            j(1)=i1
                            j(2)=i2
                            j(3)=i3
                            j(4)=i4
                            coef = y(i)%T.sub.j
                            if (coef.ne.zero) then
                               map_coor(1)=coef
                               map_coor(2)=i
                               map_coor(3)=c_%npara
                               map_coor(4)=no
                               map_coor(5)=j(1)
                               map_coor(6)=j(2)
                               map_coor(7)=j(3)
                               map_coor(8)=j(4)
                               map_coor(9)=0
                               map_coor(10)=0
                               call vector_to_table("map_table ", 'coef ', i_map_coor, map_coor(1))
                               call augment_count("map_table ")
                            endif
                         endif
                      enddo
                   enddo
                enddo
             enddo
          else
             call fort_warn('ptc_normal ','map output expects 4,5 or 6 variables')
          endif
       enddo
    enddo





    deallocate(j)



  end subroutine makemaptable


  !_________________________________________________________________

  subroutine killsavedmaps
    implicit none
    integer i,ii

    if(.not. savemaps) return

    if (.not. associated(maps)) then
       return
    endif

    do i=lbound(maps,1),ubound(maps,1)
       do ii=1,6
          call kill(maps(i)%unimap(ii))
       enddo
    enddo
    deallocate(maps)
    nullify(maps)

  end subroutine killsavedmaps
  !_________________________________________________________________


  SUBROUTINE ptc_read_errors()
    use twtrrfi
    use name_lenfi
    implicit none
    integer i,k,pos,nfac(maxmul),flag,string_from_table,double_from_table,l
    real(dp) d(2*maxmul),b(maxmul),a(maxmul),tilt,ab,bvk
    character(name_len) name,name2
    type(fibre),pointer :: p
    logical(lp) :: overwrite
    real(kind(1d0)) get_value
    character*4 :: mag_index1(10)=(/'k0l ','k1l ','k2l ','k3l ','k4l ','k5l ','k6l ','k7l ','k8l ','k9l '/)
    character*5 :: mag_index2(10)=(/'k0sl ','k1sl ','k2sl ','k3sl ','k4sl ','k5sl ','k6sl ','k7sl ','k8sl ','k9sl '/)
    character*5 :: mag_index3(11)=(/'k10l ','k11l ','k12l ','k13l ','k14l ','k15l ','k16l ','k17l ','k18l ','k19l ','k20l '/)
    character*6 :: mag_index4(11)=(/'k10sl ','k11sl ','k12sl ','k13sl ','k14sl ','k15sl ','k16sl ', &
         'k17sl ','k18sl ','k19sl ','k20sl '/)

    overwrite = get_value('ptc_read_errors ','overwrite ').ne.0
    bvk=get_value('probe ','bv ')

    nfac(1)=1
    do i=2,maxmul
       nfac(i)=nfac(i-1)*(i-1)
    enddo

    flag = string_from_table('errors_read ', 'name ',1,name)

    if(flag.ne.0) call aafail('fill_errors reports: ',' The >>> errors_read <<< table is empty ')
    i=0

    p=>my_ring%start
    do while(.true.)
       i=i+1
       a(:)=zero
       b(:)=zero
       d(:)=zero
       name2=" "
       flag = string_from_table('errors_read ', 'name ',i,name2)
       if(flag.ne.0) goto 100
       do k=1,maxmul
          if(k<=10) then
             flag = double_from_table('errors_read ',mag_index1(k),i,d(2*k-1))
             flag = double_from_table('errors_read ',mag_index2(k),i,d(2*k))
          else
             flag = double_from_table('errors_read ',mag_index3(k-10),i,d(2*k-1))
             flag = double_from_table('errors_read ',mag_index4(k-10),i,d(2*k))
          endif
       enddo
       if(flag.ne.0) goto 100
       do k=1,maxmul
          b(k)=d(2*k-1)/nfac(k)
          a(k)=d(2*k)/nfac(k)
       enddo
       name=" "
       name(:len_trim(name2)-1)=name2(:len_trim(name2)-1)
       call context(name)
       call move_to(my_ring,p,name,pos)
       tilt=-p%mag%p%tiltd
       if(pos/=0.and.p%mag%parent_fibre%dir==1) then
          if(p%mag%l/=zero) then
             do k=1,maxmul
                b(k)=b(k)/p%mag%l
                a(k)=a(k)/p%mag%l
             enddo
          endif
          if(tilt/=zero) then
             do k=1,maxmul
                ab=b(k)
                b(k)=b(k)*cos(tilt*k)+a(k)*sin(tilt*k)
                a(k)=-ab*sin(tilt*k)+a(k)*cos(tilt*k)
             enddo
          endif
          do k=1,maxmul
             b(k)=bvk*b(k)
             a(k)=bvk*a(k)
          enddo
          do k=NMAX,1,-1
             if(b(k)/=zero) then
                if(overwrite) then
                   call add(p,k,0,b(k))
                else
                   call add(p,k,1,b(k))
                endif
             endif
             if(a(k)/=zero) then
                if(overwrite) then
                   call add(p,-k,0,a(k))
                else
                   call add(p,-k,1,a(k))
                endif
             endif
          enddo
       else
          write(6,*) " name,pos, dir of dna ",name, p%mag%parent_fibre%dir
       endif
    enddo
100 continue
    return

  end SUBROUTINE ptc_read_errors

END MODULE madx_ptc_module
