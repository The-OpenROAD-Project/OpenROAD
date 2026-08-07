# Design Space Exploration (DSE) & Automated Tuning

OpenROAD supports automated Design Space Exploration (DSE) tools and autotuners across all stages of physical design.

## 1. The Autotuner Misconception

The plausible-sounding story of DSE goes: *We have a flow. It takes parameters. It produces a WNS and an area number. We have an autotuner. Point the tuner at the flow, give it a machine budget, come back in the morning.*

When this approach yields mediocre results, users often mistakenly conclude that DSE is oversold. The reality is that the initial disappointment is due to mismanaged expectations: **OpenROAD cannot have an opinion on what makes your product perfect.**

OpenROAD and ORFS are designed to translate RTL into physical layout and extract Power, Performance, and Area (PPA) metrics. However, true product optimization requires evaluating system-level properties—such as "useful work done per cycle" or "quality of results"—which are properties of the RTL combined with the software workload. **Only the user can write the optimization function for their design.**

To successfully use DSE, the user must define an external optimization function that evaluates OpenROAD's PPA outputs against application-specific metrics. For example:

* **General Purpose CPUs:** The optimization function typically maximizes throughput (e.g., Instructions Per Cycle for a defined software workload) given strict PPA constraints (frequency targets, thermal limits). OpenROAD determines if that specific RTL architecture is physically realizable and what its power cost is; the optimizer balances the two.
* **Machine Learning Accelerators:** In hardware-software co-design, the true optimization function is often **Inference-per-Watt** (successful inferences per joule of energy), subject to latency and accuracy constraints. OpenROAD provides power and area estimates for the hardware architecture, while the software side (network pruning, quantization) dictates accuracy and throughput. The perfect design is found by searching across both domains simultaneously.

## 2. Rungs and Populations: Climbing the Flow

A stock production flow runs for an hour and reports at the end. For an autotuner, burning an hour to find out a parameter configuration is invalid is a terrible exchange rate. The solution is to use early stopping points (rungs on a ladder), but this introduces new challenges.

### The Ladder of Estimates
It is a **second misconception** that the standard flow (Synthesis → Floorplan → Place → CTS → GRT) is the *only* progression of rungs. The signal that drives DSE does not have to be the production flow itself.

Users can, and often must, write their own progression of rungs with custom signals designed specifically to find the search domain. Even if these custom signals are completely unrealistic compared to the final flow, they can be incredibly useful. 

For example, a custom estimation script might run floorplan and global placement, and then estimate parasitics *without* running incremental timing repair. This skipped repair means wires will get a massive penalty, producing a WNS that is entirely unrealistic compared to a true GRT signoff. However, this cheap, "broken" signal can provide enough feedback for the DSE tuner to rapidly find the right floorplan shape without spending hours on placement repair. Eventually, the full flow must be run, but custom, "unrealistic" rungs are essential for pruning the search space cheaply.

Stop points roughly follow this ladder, though you should interleave your own custom signals:
* **Quick Synthesis:** Cell count, logic depth.
* **Full Synthesis:** Area, rough timing.
* **Custom Estimators (e.g. `estimate.tcl`):** Rough layout feasibility (no repair).
* **Global Place:** Congestion, utilization.
* **Place + Opt:** Timing with ideal clocks.
* **CTS:** Real skew.
* **Global Route (GRT):** Accurate wire delay and routing feasibility.
* **Signoff (Detailed Route + RCX):** The truth.

### The Trap: Correlation vs. Retention
The obvious move is to check whether the cheap early number correlates with the final number. If a rank correlation is low, the early stage looks useless. 

**This is the wrong measurement.** You don't need the cheap stage to *rank* candidates perfectly; you need it to **not throw away the good ones**. A stage might be terrible at distinguishing the 12th best config from the 37th, but perfectly reliable at keeping the true top 10 somewhere inside its own top 50.

To calibrate your ladder, measure retention on an anchor set (a set of designs where you ran the full flow): *Of the 10 designs that were genuinely best at signoff, how many were in this stage's top 50?* That number sets your "keep-width" for the rung.

### Cull Candidates, Don't Shrink the Search Space
When an autotuner gets early information, it can drop specific candidates or it can narrow the range of parameters it samples from. **These are not symmetric.**

Because early stages (like post-synth area) are biased relative to final stages (like post-place area, where legalization adds cells), early rankings are noisy.
**Rule:** Let cheap stages kill candidates, but never let them shrink the search box. Only a stage that passes a strict recall check earns the right to narrow the search bounds. Dropping a candidate is recoverable by sampling the region again; narrowing the bounds permanently blinds the tuner to that region.

## 3. Custom DSE Signals & Single-Process Execution

If users require a different signal than the full flow, they should write their own `.tcl` script. ORFS natively supports running the entire flow in a single OpenROAD process execution (e.g., via `KEEP_VARS=1` and `WRITE_ODB_AND_SDC_EACH_STAGE=0`, utilizing `flow.tcl`).

Users are encouraged to write custom flows reusing existing ORFS `.tcl` steps or skipping them entirely. For example, a custom estimator script could run `floorplan`, `global_placement`, and a fast 2-iteration `global_route -congestion_iterations 2` to yield a rapid signal in minutes rather than waiting hours for the full flow to complete. This provides full flexibility tailored to a specific use-case without requiring OpenROAD to anticipate all requirements.

## 4. Framework Inversion with bazel-orfs

For advanced DSE, `bazel-orfs` introduces **framework inversion**. While Bazel acts as a rigid, reproducible build system that pins tools and containers, it can be leveraged to produce a self-contained script where parameterized RTL is injected at the top. 

* **Dynamic Adaptation:** Using Bazel, you can intercept the synthesis database to dynamically calculate optimal `CORE_UTILIZATION` or floorplan parameters before the floorplan stage runs, preventing doomed runs before they start.
* **Separation of Concerns:** Bazel can stay out of the DSE execution loop entirely. This means an orchestrator like Ray can sit *on top* (calling Bazel for hermetic signoff builds) or *at the bottom* (executing the Bazel-built self-contained scripts on a cluster). 
* **Custom Patches:** Users can carry local patches in Bazel to extend OpenROAD on-the-fly, implementing design-specific early-termination policies. OpenROAD and ORFS aim to enable whichever framework arrangement is right for the user and project.

## 5. Literature & Prior Art

The architectural concepts described in this document—using cheap, inaccurate signals to prune a search space before committing to expensive simulations—are not unique to EDA. These methods are rigorously studied across various engineering disciplines under the umbrella of **Multi-Fidelity Optimization (MFO)** and **Surrogate Modeling**.

If you are building an autotuner for OpenROAD, examining prior art in these fields can provide robust mathematical frameworks for managing your "ladder of estimates":

* **Multi-Fidelity Optimization in Aerospace:** In aerospace engineering, optimizing an aircraft wing requires balancing cheap 2D potential flow simulations (minutes) against expensive 3D Navier-Stokes Computational Fluid Dynamics (days). Research here focuses heavily on *Cokriging* and *Multi-Fidelity Gaussian Processes* (MF-GP), which learn the correlation (and discrepancy) between low-fidelity models and high-fidelity ground truths to intelligently guide the search.
* **Hyperparameter Optimization in Machine Learning:** The concept of early-stopping and culling bad candidates dynamically is the foundation of algorithms like **Hyperband** and **ASHA (Asynchronous Successive Halving Algorithm)**. These algorithms allocate compute budgets aggressively, killing off the worst-performing configurations early in their training epochs and promoting only the promising ones to full completion.
* **Bayesian Optimization (MFBO):** Multi-fidelity Bayesian Optimization frameworks use specialized acquisition functions (like *Entropy Search* or *Information-Theoretic* approaches) to decide whether the tuner should query a cheap, noisy estimator (like `estimate.tcl`) or pay the cost for a full signoff run to maximize information gain per compute hour.
