void PhysRemap::Remap()
{
  if (script_) {
#ifdef DEBUG_RESTRUCT
    printf("Applying script %s\n", script_to_apply_.c_str());
#endif
  }
  // Build the abc network for the cut. This is sta -> abc conversion
  Cut2Ntk c2nwk(logger_, nwk_, cut_, cut_name_);
  Abc_Ntk_t* sop_logic_nwk = c2nwk.BuildSopLogicNetlist();

  // Annotate the timing requirements. We have found the blif
  // interface has weird problems with annotating timing
  // constraints so we do it directly on the network.
  AnnotateTimingRequirementsOntoABCNwk(sop_logic_nwk);

  // Now apply abc commands directly (if given)
  // else remap the logic after structure hashing and balancing
  Abc_Ntk_t* cut_logic = Abc_NtkToLogic(sop_logic_nwk);

#ifdef REGRESS_RESTRUCT
  Abc_NtkSetName(cut_logic, Extra_UtilStrsav(cut_name_.c_str()));
  printf("Writing out cut %s for regression\n", cut_name_.c_str());
  std::string op_name = "ip_.blif";
  Io_Write(cut_logic, const_cast<char*>(op_name.c_str()), IO_FILE_BLIF);
#endif

  if (script_) {
    Abc_Ntk_t* pNtk = Abc_NtkStrash(cut_logic, 0, 0, 0);
    Abc_FrameReplaceCurrentNetwork(Abc_FrameGetGlobalFrame(), pNtk);
    int status = 1;
    status = Cmd_CommandExecute(Abc_FrameGetGlobalFrame(),
                                script_to_apply_.data());
    if (status == 0) {
      // get the mapped netlist from the global frame
      // make sure the final result is mapped !
      Abc_Ntk_t* mapped_netlist = Abc_FrameReadNtk(Abc_FrameGetGlobalFrame());
      intermediate_result_ = mapped_netlist;
      if (accept_)
        BuildCutRealization(mapped_netlist);
    } else {
#ifdef DEBUG_RESTRUCT
      printf("Error running script %s\n", script_to_apply_.c_str());
#endif
    }
  } else {
    // default: just low level remapping.
    Abc_Ntk_t* pNtk = Abc_NtkStrash(cut_logic, 0, 0, 0);
    Abc_Ntk_t* pNtk_res = Abc_NtkBalance(pNtk, 0, 0, 1);
    Amap_Par_t Pars_local;
    Amap_ManSetDefaultParams(&Pars_local);
    // favour timing
    Pars_local.fFreeInvs = 0;   // favour timing
    Pars_local.fADratio = 9.9;  // favour timing
    Pars_local.fVerbose = 0;
    // remap
    Abc_Ntk_t* mapped = Abc_NtkDarAmap(pNtk_res, &Pars_local);
    Abc_Ntk_t* mapped_netlist = Abc_NtkToNetlist(mapped);
    intermediate_result_ = mapped_netlist;
#ifdef REGRESS_RESTRUCT
    {
      if (!Abc_NtkHasAig(mapped_netlist) && !Abc_NtkHasMapping(mapped_netlist))
        Abc_NtkToAig(mapped_netlist);
      Abc_Ntk_t* result_logic = Abc_NtkToLogic(mapped_netlist);
      std::string op_name = "op_.blif";
      Io_Write(result_logic, const_cast<char*>(op_name.c_str()), IO_FILE_BLIF);
    }
#endif

    // stitch the result back into the physical
    // netlist, if we are instructed to do so.
    // Else let parent optimizer decide.
    if (accept_)
      BuildCutRealization(mapped_netlist);
  }
}

void PhysRemap::AnnotateTimingRequirementsOntoABCNwk(Abc_Ntk_t* sop_logic_nwk)
{
  // Axiom the order in which the abc pi/po is made is :
  // inputs: leaf order in vector
  // outputs: root order in vector
  // So leaf at [0] corresponds to first abc CI.
  // So element at [leaf_count.size()] is first abc CO.
  int ip_ix = 0;
  int op_ix = 0;
  int ix = 0;
  // timing requirements are set in PhysTiming.cc/.hh
  sta::Sta* sta = sta::Sta::sta();
#ifdef REGRESS_RESTRUCT
  FILE* timing_file = fopen("timing.txt", "w+");
#endif
  for (auto tr : timing_requirements_) {
    const sta::Pin* sta_pin = tr.first;
    TimingRecord* trec = tr.second;
    // inputs set arrival time
    if (ix < cut_->leaves_.size() && trec) {
      ip_ix = ix;
      Abc_Obj_t* pi = (Abc_Obj_t*) (Vec_PtrEntry(sop_logic_nwk->vCis, ip_ix));
#ifdef REGRESS_RESTRUCT
      fprintf(timing_file,
              "Ip %d Arr  r: %s f: %s \n",
              pi->Id,
              delayAsString(trec->arrival_rise, sta),
              delayAsString(trec->arrival_fall, sta));
#endif
      Abc_NtkTimeSetArrival(
          sop_logic_nwk, pi->Id, trec->arrival_rise, trec->arrival_fall);
    }
    // outputs set required time
    else {
      if (trec) {
        op_ix = ix - cut_->leaves_.size();
        Abc_Obj_t* po = (Abc_Obj_t*) (Vec_PtrEntry(sop_logic_nwk->vCos, op_ix));
        Abc_NtkTimeSetRequired(
            sop_logic_nwk, po->Id, trec->required_rise, trec->required_fall);
#ifdef REGRESS_RESTRUCT
        fprintf(timing_file,
                "Op %d Rqd  r: %s f: %s\n",
                po->Id,
                delayAsString(trec->required_rise, sta),
                delayAsString(trec->required_fall, sta));
#endif
      }
    }
    ix++;
  }
#ifdef REGRESS_RESTRUCT
  fclose(timing_file);
#endif
}

bool PhysRemap::getAbcResult(abc::Abc_Ntk_t_*& result)
{
  if (intermediate_result_) {
    result = intermediate_result_;
    return true;
  }
  return false;
}

float PhysRemap::TimingGain()
{
  std::map<int, float> required_time;
  std::map<int, float> arrival_time;
  Abc_Obj_t* po;
  int op_ix = 0;
  float min_rise_gain = 0.0;
  float min_fall_gain = 0.0;
  Abc_NtkForEachPo(intermediate_result_, po, op_ix)
  {
    Abc_Time_t_* arrival_time = Abc_NodeReadArrival(po);
    Abc_Time_t_* required_time = Abc_NodeReadRequired(po);
    if (arrival_time && required_time) {
      float rise_gain = required_time->Rise - arrival_time->Rise;
      float fall_gain = required_time->Fall - arrival_time->Fall;
      min_rise_gain = rise_gain > min_rise_gain ? rise_gain : min_rise_gain;
      min_fall_gain = fall_gain > min_fall_gain ? fall_gain : min_fall_gain;
    }
  }
  if (min_rise_gain > min_fall_gain)
    return min_rise_gain;
  return min_fall_gain;
}

void PhysRemap::BuildCutRealization(Abc_Ntk_t* nwk)
{
  // Build the cut in the network.
  // And delete the original volume.
  Ntk2Cut n2c(nwk, cut_, nwk_);
  n2c.BuildNwkElements();
}
