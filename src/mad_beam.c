#include "madx.h"

#if 0 // not used...
static double
get_beam_value(char* name, char* par)
  /* this function is used by fortran to get the parameters values of beams;
     returns parameter value "par" for beam of sequence "name" if present,
     where "name" may be "current", or "default", or the name of a sequence;
     else returns INVALID */
{
  struct command* cmd;
  mycpy(c_dum->c, name);
  mycpy(aux_buff->c, par);
  if (strcmp(c_dum->c, "current") == 0 && current_beam != NULL)
    return get_value("beam", par);
  else if (strcmp(c_dum->c, "default") == 0)
  {
    cmd = find_command("default_beam", beam_list);
    return command_par_value(aux_buff->c, cmd);
  }
  else if ((cmd = find_command(c_dum->c, beam_list)) != NULL)
    return command_par_value(aux_buff->c, cmd);
  else return INVALID;
}
#endif

// public interface

void
exec_beam(struct in_cmd* cmd, int flag)
  /* chooses correct beam for beam definitions, upgrades, and resets */
{
  char* name;
  char name_def[] = "default_beam";
  struct command* keep_beam = current_beam;
  struct command_parameter_list* pl = cmd->clone->par;
  struct name_list* nl = cmd->clone->par_names;
  int pos = name_list_pos("sequence", nl);
  int bpos = name_list_pos("sequence", current_beam->par_names);
  if (nl->inform[pos])
  {
    name = pl->parameters[pos]->string;
    if ((current_beam = find_command(name, beam_list)) == NULL)
    {
      set_defaults("beam");
      add_to_command_list(name, current_beam, beam_list, 0);
    }
  }
  else
  {
    name = name_def;
    current_beam = find_command(name, beam_list);
  }
  current_beam->par->parameters[bpos]->string = permbuff(name);
  current_beam->beam_def = 1;
  if (flag == 0) update_beam(cmd->clone);
  else if (flag == 1)  set_defaults("beam");
  current_beam = keep_beam;
}

void
save_beam(struct sequence* sequ, FILE* file)
{
  struct command* comm;
  char beam_buff[AUX_LG];
  int i, def = 0;
  if ((comm = find_command(sequ->name, beam_list)) == NULL)
  {
    if (default_beam_saved == 0)
    {
      def = default_beam_saved = 1;
      comm = find_command("default_beam", beam_list);
    }
  }
  if (comm != NULL)
  {
    beam_buff[0] = '\0';
    strcat(beam_buff, "beam");
    for (i = 0; i < comm->par->curr; i++)
    {
      if (comm->par_names->inform[i])
      {
        if (strcmp(comm->par_names->names[i], "sequence") != 0
            || def == 0)
          export_comm_par(comm->par->parameters[i], beam_buff);
      }
    }
    write_nice(beam_buff, file);
  }
}

void
show_beam(char* tok)
{
  struct command* comm;
  if (strlen(tok) > 5 && tok[4] == '%')
    comm = find_command(&tok[5], beam_list);
  else comm = find_command("default_beam", beam_list);
  if (comm != NULL) dump_command(comm);
}

void
update_beam(struct command* comm)
  /* calculates consistent values for modified beam data set.
     beam command values are evaluated in the order:
     particle->(mass+charge)
     energy->pc->gamma->beta
     ex->exn
     ey->eyn
     current->npart
     et->sigt->sige
     where any item to the left takes precendence over the others;
     for ions, the input energy is multiplied by the charge, and the
  */
{
  struct name_list* nlc = comm->par_names;
  struct command_parameter_list* plc = comm->par;
  struct command_parameter_list* pl = current_beam->par;
  int pos, lp;
  char* name = blank;
  double energy = 0, beta = 0, gamma = 0, charge = 0, freq0 = 0, bcurrent = 0,
    npart = 0, mass = 0, pc = 0, ex, exn, ey, eyn, alfa, circ = 0,
    arad = 0;

  pos = name_list_pos("particle", nlc);
  if (nlc->inform[pos])  /* parameter has been read */
    {
      pl->parameters[pos]->string = name
	= plc->parameters[pos]->string;
      if ((lp = name_list_pos(name, defined_commands->list)) > -1)
	{
	  mass = command_par_value("mass", defined_commands->commands[lp]);
	  charge = command_par_value("charge", defined_commands->commands[lp]);
	}
      else /* unknown particle, then mass and charge must be given as well */
	{
	  pos = name_list_pos("mass", nlc);
	  if (nlc->inform[pos]) mass = command_par_value("mass", comm);
	  else
	    {
	      warning("emass given to unknown particle:", name);
	      mass = get_variable("emass");
	    }
	  pos = name_list_pos("charge", nlc);
	  if (nlc->inform[pos]) charge = command_par_value("charge", comm);
	  else
	    {
	      warning("charge +1 given to unknown particle:", name);
	      charge = 1;
	    }
	}
    }
  else if (nlc->inform[name_list_pos("mass", nlc)])
    {
      mass = command_par_value("mass", comm);
      pl->parameters[pos]->string = name = permbuff("default");
      pos = name_list_pos("charge", nlc);
      if (nlc->inform[pos]) charge = command_par_value("charge", comm);
      else
	{
	  warning("charge +1 given to user particle:", name);
	  charge = 1;
	}
    }
  else name = pl->parameters[pos]->string;

  if (strcmp(name, "ion") == 0)
    {
      pos = name_list_pos("mass", nlc);
      if (nlc->inform[pos]) mass = command_par_value("mass", comm);
      pos = name_list_pos("charge", nlc);
      if (nlc->inform[pos]) charge = command_par_value("charge", comm);
      else charge = command_par_value("charge", current_beam);
    }

  if (mass == zero) mass = command_par_value("mass", current_beam);

  if (charge == zero) charge = command_par_value("charge", current_beam);

  arad = ten_m_16 * charge * charge * get_variable("qelect")
    * clight * clight / mass;

  if ((pos = name_list_pos("energy", nlc)) > -1 && nlc->inform[pos])
    {
      energy = command_par_value("energy", comm);
      if (energy <= mass) fatal_error("energy must be","> mass");
      pc = sqrt(energy*energy - mass*mass);
      gamma = energy / mass;
      beta = pc / energy;
    }
  else if((pos = name_list_pos("pc", nlc)) > -1 && nlc->inform[pos])
    {
      pc = command_par_value("pc", comm);
      energy = sqrt(pc*pc + mass*mass);
      gamma = energy / mass;
      beta = pc / energy;
    }
  else if((pos = name_list_pos("gamma", nlc)) > -1 && nlc->inform[pos])
    {
      if ((gamma = command_par_value("gamma", comm)) <= one)
	fatal_error("gamma must be","> 1");
      energy = gamma * mass;
      pc = sqrt(energy*energy - mass*mass);
      beta = pc / energy;
    }
  else if((pos = name_list_pos("beta", nlc)) > -1 && nlc->inform[pos])
    {
      if ((beta = command_par_value("beta", comm)) >= one)
	fatal_error("beta must be","< 1");
      gamma = one / sqrt(one - beta*beta);
      energy = gamma * mass;
      pc = sqrt(energy*energy - mass*mass);
    }
  else
    {
      energy = command_par_value("energy", current_beam);
      if (energy <= mass) fatal_error("energy must be","> mass");
      pc = sqrt(energy*energy - mass*mass);
      gamma = energy / mass;
      beta = pc / energy;
    }

  if (nlc->inform[name_list_pos("ex", nlc)])
    {
      ex = command_par_value("ex", comm);
      exn = ex * 4 * beta * gamma;
    }
  else if (nlc->inform[name_list_pos("exn", nlc)])
    {
      exn = command_par_value("exn", comm);
      ex = exn / (4 * beta * gamma);
    }
  else
    {
      ex = command_par_value("ex", current_beam);
      exn = ex * 4 * beta * gamma;
    }

  if (nlc->inform[name_list_pos("ey", nlc)])
    {
      ey = command_par_value("ey", comm);
      eyn = ey * 4 * beta * gamma;
    }
  else if (nlc->inform[name_list_pos("eyn", nlc)])
    {
      eyn = command_par_value("eyn", comm);
      ey = eyn / (4 * beta * gamma);
    }
  else
    {
      ey = command_par_value("ey", current_beam);
      eyn = ey * 4 * beta * gamma;
    }

  alfa = one / (gamma * gamma);

  if (nlc->inform[name_list_pos("circ", nlc)])
    {
      circ = command_par_value("circ", comm);
      if (circ > zero) freq0 = (beta * clight) / (ten_p_6 * circ);
    }
  else if (nlc->inform[name_list_pos("freq0", nlc)])
    {
      freq0 = command_par_value("freq0", comm);
      if (freq0 > zero) circ = (beta * clight) / (ten_p_6 * freq0);
    }
  else if ((pos = name_list_pos(name, sequences->list)) >= 0)
    {
      circ = sequence_length(sequences->sequs[pos]);
      freq0 = (beta * clight) / (ten_p_6 * circ);
    }

  if (nlc->inform[name_list_pos("bcurrent", nlc)])
    {
      bcurrent = command_par_value("bcurrent", comm);
      if (bcurrent > zero && freq0 > zero)
	npart = bcurrent / (beta * freq0 * ten_p_6 * get_variable("qelect"));
      else if (nlc->inform[name_list_pos("npart", nlc)])
	{
	  npart = command_par_value("npart", comm);
	  bcurrent = npart * beta * freq0 * ten_p_6 * get_variable("qelect");
	}
    }

  else if (nlc->inform[name_list_pos("npart", nlc)])
    {
      npart = command_par_value("npart", comm);
      bcurrent = npart * beta * freq0 * ten_p_6 * get_variable("qelect");
    }

  pos = name_list_pos("bunched", nlc);
  if (nlc->inform[pos])
    pl->parameters[pos]->double_value = plc->parameters[pos]->double_value;

  pos = name_list_pos("radiate", nlc);
  if (nlc->inform[pos])
    pl->parameters[pos]->double_value = plc->parameters[pos]->double_value;
  
  pos = name_list_pos("et", nlc);
  if (nlc->inform[pos])
    pl->parameters[pos]->double_value = plc->parameters[pos]->double_value;

  pos = name_list_pos("sigt", nlc);
  if (nlc->inform[pos])
    pl->parameters[pos]->double_value = plc->parameters[pos]->double_value;

  pos = name_list_pos("sige", nlc);
  if (nlc->inform[pos])
    pl->parameters[pos]->double_value = plc->parameters[pos]->double_value;

  pos = name_list_pos("kbunch", nlc);
  if (nlc->inform[pos])
    pl->parameters[pos]->double_value = plc->parameters[pos]->double_value;

  pos = name_list_pos("bv", nlc);
  if (nlc->inform[pos])
    pl->parameters[pos]->double_value = plc->parameters[pos]->double_value;

  pos = name_list_pos("pdamp", nlc);
  if (nlc->inform[pos])
    copy_double(plc->parameters[pos]->double_array->a, pl->parameters[pos]->double_array->a, 3);

  store_comm_par_value("mass", mass, current_beam);
  store_comm_par_value("charge", charge, current_beam);
  store_comm_par_value("energy", energy, current_beam);
  store_comm_par_value("pc", pc, current_beam);
  store_comm_par_value("gamma", gamma, current_beam);
  store_comm_par_value("ex", ex, current_beam);
  store_comm_par_value("exn", exn, current_beam);
  store_comm_par_value("ey", ey, current_beam);
  store_comm_par_value("eyn", eyn, current_beam);
  store_comm_par_value("npart", npart, current_beam);
  store_comm_par_value("bcurrent", bcurrent, current_beam);
  store_comm_par_value("freq0", freq0, current_beam);
  store_comm_par_value("circ", circ, current_beam);
  store_comm_par_value("beta", beta, current_beam);
  store_comm_par_value("alfa", alfa, current_beam);
  store_comm_par_value("arad", arad, current_beam);
}

void
adjust_beam(void)
  /* adjusts beam parameters to current beta, gamma, bcurrent, npart */
{
  struct name_list* nl = current_beam->par_names;
  double circ = one, freq0, alfa, beta, gamma, bcurrent = zero, npart = 0;
  if (current_sequ != NULL && sequence_length(current_sequ) != zero)
    circ = current_sequ->length;
  beta = command_par_value("beta", current_beam);
  gamma = command_par_value("gamma", current_beam);
  alfa = one / (gamma * gamma);
  freq0 = (beta * clight) / (ten_p_6 * circ);
  if (nl->inform[name_list_pos("bcurrent", nl)] &&
      (bcurrent = command_par_value("bcurrent", current_beam)) > zero)
    npart = bcurrent / (freq0 * ten_p_6 * get_variable("qelect"));
  else if (nl->inform[name_list_pos("npart", nl)] &&
           (npart = command_par_value("npart", current_beam)) > zero)
    bcurrent = npart * freq0 * ten_p_6 * get_variable("qelect");

  store_comm_par_value("alfa", alfa, current_beam);
  store_comm_par_value("freq0", freq0, current_beam);
  store_comm_par_value("circ", circ, current_beam);
  store_comm_par_value("npart", npart, current_beam);
  store_comm_par_value("bcurrent", bcurrent, current_beam);
}

int
attach_beam(struct sequence* sequ)
  /* attaches the beam belonging to the current sequence */
{
  if (!sequ || (current_beam = find_command(sequ->name, beam_list)) == NULL)
    current_beam = find_command("default_beam", beam_list);
  return current_beam->beam_def;
}

