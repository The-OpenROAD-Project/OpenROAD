# Design Space Exploration (DSE) & Automated Tuning

OpenROAD supports automated Design Space Exploration (DSE) tools and autotuners across all stages of physical design.

This guide explores architectural patterns and concepts for advanced Design Space Exploration (DSE) and large-scale autotuning. While many users achieve their goals using the standard OpenROAD flow and default metrics, building automated tuning frameworks often introduces unique challenges. This document provides background on how these advanced search spaces can be managed.

## 1. The Autotuner Misconception

The plausible-sounding story of DSE goes: *We have a flow. It takes parameters. It produces a WNS and an area number. We have an autotuner. Point the tuner at the flow, give it a machine budget, come back in the morning.*

The misconception here is the assumption that a normal full flow is all there is, and that it must always be used as an optimization function. In reality, different approaches exist, and neither is universally "best."

An **autotuner** is designed to run the whole flow and optimize parameters for it. This is a standard use-case for evaluating the complete pipeline and extracting final sign-off metrics.

However, if your use-case requires running your own optimization function, you can put together your own customized optimization flow, perhaps as a single `.tcl` script. In this scenario, you are probably interested in getting a speedy result—such as a clock frequency outside of the domain parameters for the physical codomain. You care more about converging quickly towards interesting domain parameters than having accurate, non-physical codomain values.

OpenROAD aims to state what these use-cases are and let you pick the one that fits your project. It has no opinion on what makes your product perfect; only the user can write the optimal function for their design.


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
**Strategy:** Let cheap stages kill candidates, but never let them shrink the search box. Only a stage that passes a strict recall check earns the right to narrow the search bounds. Dropping a candidate is recoverable by sampling the region again; narrowing the bounds permanently blinds the tuner to that region.

## 3. Custom DSE Signals & Single-Process Execution

If users require a different signal than the full flow, they should write their own `.tcl` script. ORFS natively supports running the entire flow in a single OpenROAD process execution (e.g., via `KEEP_VARS=1` and `WRITE_ODB_AND_SDC_EACH_STAGE=0`, utilizing `flow.tcl`).

For advanced DSE, custom flows can be written to reuse existing ORFS `.tcl` steps or skip them entirely. (Note: The standard ORFS flow remains the standard path for general use). For example, a custom estimator script could run `floorplan`, `global_placement`, and a fast 2-iteration `global_route -congestion_iterations 2` to yield a rapid signal in minutes rather than waiting hours for the full flow to complete. This provides full flexibility tailored to a specific use-case without requiring OpenROAD to anticipate all requirements.

## 4. The Contract Between Intent and Implementation

Writing and iterating on defensive `.tcl` scripts to catch fatal flow errors is a massive cognitive burden with unacceptable turnaround times. When a `.tcl` script takes hours to run, debugging missing `catch` blocks or manually writing code to poll futility metrics paralyzes iteration. 

To solve this, OpenROAD enforces a strict separation of concerns:
* **`.tcl` Scripts (Intent):** The flow scripts should purely express the declarative intent of the designer (e.g., "run global placement, then global routing").
* **C++ Engine (Implementation):** The underlying OpenROAD C++ solvers are responsible for executing that intent gracefully. If a design is doomed (futility), the engine must recognize this, immediately short-circuit, and degrade gracefully (warning the user and returning a success code instead of a fatal crash). This prevents the tool from stubbornly grinding for hours on a hopeless design and allows the pipeline to continue seamlessly.

**The AI Development Contract:** This architectural boundary is not just a policy for human developers; it is a strict contract for AI coding agents. As established in the Google Antigravity documentation, agentic AI fundamentally requires this strict separation of concerns and hard stops on futility to function effectively. Without it, agents are forced to hack around in `.tcl` scripts to manage futility, where massive turnaround times destroy the agent's context window and iteration loop. We actively dogfood this policy: AI agents use this exact contract to develop and fix `bazel-orfs`, OpenROAD C++, and `.tcl` estimator scripts.

## 5. Framework Inversion with bazel-orfs

For advanced DSE, `bazel-orfs` introduces **framework inversion**. While Bazel acts as a rigid, reproducible build system that pins tools and containers, it can be leveraged to produce a self-contained script where parameterized RTL is injected at the top. 

* **Dynamic Adaptation:** Using Bazel, you can intercept the synthesis database to dynamically calculate optimal `CORE_UTILIZATION` or floorplan parameters before the floorplan stage runs, preventing doomed runs before they start.
* **Separation of Concerns:** Bazel can stay out of the DSE execution loop entirely. This means an orchestrator like Ray can sit *on top* (calling Bazel for hermetic signoff builds) or *at the bottom* (executing the Bazel-built self-contained scripts on a cluster). 
* **Custom Patches:** Users can carry local patches in Bazel to extend OpenROAD on-the-fly, implementing design-specific early-termination policies. OpenROAD and ORFS aim to enable whichever framework arrangement is right for the user and project.

## 6. Literature & Prior Art

The architectural concepts described in this document—using cheap, inaccurate signals to prune a search space before committing to expensive simulations—are not unique to EDA. These methods are rigorously studied across various engineering disciplines under the umbrella of **Multi-Fidelity Optimization (MFO)** and **Surrogate Modeling**.

If you are building an autotuner for OpenROAD, examining prior art in these fields can provide robust mathematical frameworks for managing your "ladder of estimates":

* **Multi-Fidelity Optimization in Aerospace:** In aerospace engineering, optimizing an aircraft wing requires balancing cheap 2D potential flow simulations (minutes) against expensive 3D Navier-Stokes Computational Fluid Dynamics (days). Research here focuses heavily on *Cokriging* and *Multi-Fidelity Gaussian Processes* (MF-GP), which learn the correlation (and discrepancy) between low-fidelity models and high-fidelity ground truths to intelligently guide the search.
* **Hyperparameter Optimization in Machine Learning:** The concept of early-stopping and culling bad candidates dynamically is the foundation of algorithms like **Hyperband** and **ASHA (Asynchronous Successive Halving Algorithm)**. These algorithms allocate compute budgets aggressively, killing off the worst-performing configurations early in their training epochs and promoting only the promising ones to full completion.
* **Bayesian Optimization (MFBO):** Multi-fidelity Bayesian Optimization frameworks use specialized acquisition functions (like *Entropy Search* or *Information-Theoretic* approaches) to decide whether the tuner should query a cheap, noisy estimator (like `estimate.tcl`) or pay the cost for a full signoff run to maximize information gain per compute hour.
