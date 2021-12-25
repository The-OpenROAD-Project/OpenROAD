# Instructions for AutoTuner with Ray

_AutoTuner_ is a "no-human-in-loop" parameter tuning framework for commercial and academic RTL-to-GDS flows. 
AutoTuner provides a generic interface where users can define parameter configuration as JSON objects. 
This enables AutoTuner to easily support various tools and flows. AutoTuner also utilizes [METRICS2.1](https://github.com/ieee-ceda-datc/datc-rdf-Metrics4ML) to capture PPA 
of individual search trials. With the abundant features of METRICS2.1, users can explore various reward functions that steer the flow autotuning to different PPA goals. 

AutoTuner provides two main functionalities as follows.
* Automatic hyperparameter tuning framework for OpenROAD-flow-script (ORFS)
* Parametric sweeping experiments for ORFS


AutoTuner contains top-level Python script for ORFS, each of which implements a different search algorithm. Current supported search algorithms are as follows.
* Random/Grid Search 
* Population Based Training ([PBT](https://deepmind.com/blog/article/population-based-training-neural-networks))
* Tree Parzen Estimator ([HyperOpt](http://hyperopt.github.io/hyperopt))
* Bayesian + Multi-Armed Bandit ([AxSearch](https://ax.dev/))
* Tree Parzen Estimator + Covariance Matrix Adaptation Evolution Strategy ([Optuna](https://optuna.org/))
* Evolutionary Algorithm ([Nevergrad](https://github.com/facebookresearch/nevergrad))

User-settable coefficient values (`coeff_perform`, `coeff_power`, `coeff_area`) of three objectives to set the direction of tuning are written in the script. Each coefficient is expressed as a global variable at the `get_ppa` function in `PPAImprov` class in the script (`coeff_perform`, `coeff_power`, `coeff_area`). Efforts to optimize each of the objectives are proportional to the specified coefficients.


## Input JSON structure

Sample JSON file for sky130hd aes design: [[link]](https://github.com/The-OpenROAD-Project/OpenROAD-flow-scripts/blob/master/flow/designs/sky130hd/aes/autotuner.json)

Simple Example:
```json
{
    "_SDC_FILE_PATH": "constraint.sdc",     
    "_SDC_CLK_PERIOD": {                    
        "type": "float",                    
        "minmax": [                         
            1.0,                            
            3.7439
        ],
        "step": 0                           
    },
    "CORE_MARGIN": {                        
        "type": "int",
        "minmax": [
            2,
            2
        ],
        "step": 0                           
    },
}
```

* `"_SDC_FILE_PATH"`, `"_SDC_CLK_PERIOD"`, `"CORE_MARGIN"`: Parameter names for sweeping/tuning. 
* `"type"`: Parameter type ("float" or "int") for sweeping/tuning
* `"minmax"`: Min-to-max range for sweeping/tuning. The unit follows the default value of each technology std cell library.
* `"step"`: Parameter step within the minmax range. Step 0 for type "float" means continuous step for sweeping/tuning. Step 0 for type "int" means the constant parameter.

## Tunable / sweepable parameters

Tables of parameters that can be swept/tuned in technology platforms supported by ORFS.
Any variable that can be set from the command line can be used for tune or sweep.

For SDC you can use:

* `_SDC_FILE_PATH`
  - Path relative to the current JSON file to the SDC file.
* `_SDC_CLK_PERIOD`
  - Design clock period. This will create a copy of `_SDC_FILE_PATH` and modify the clock period.
* `_SDC_UNCERTAINTY`
  - Clock uncertainty. This will create a copy of `_SDC_FILE_PATH` and modify the clock uncertainty.
* `_SDC_IO_DELAY`
  - I/O delay. This will create a copy of `_SDC_FILE_PATH` and modify the I/O delay.



For FastRoute you can use:

* `_FR_FILE_PATH`
  - Path relative to the current JSON file to the `fastroute.tcl` file.
* `_FR_LAYER_ADJUST`
  - Layer adjustment. This will create a copy of `_FR_FILE_PATH` and modify the layer adjustment for all routable layers, i.e., from `$MIN_ROUTING_LAYER` to `$MAX_ROUTING_LAYER`.
* `_FR_LAYER_ADJUST_NAME`
  - Layer adjustment for layer NAME. This will create a copy of `_FR_FILE_PATH` and modify the layer adjustment only for the layer NAME.
* `_FR_GR_SEED`
  - Global route random seed. This will create a copy of `_FR_FILE_PATH` and modify the global route random seed.




## How to use

### General Information

`distributed.py` scripts handles sweeping and tuning of ORFS parameters.
 
For both sweep and tune modes <mode>:
```shell
python3 distributed.py -h
```
 
Note: the order of the parameters matter. Arguments `--design`, `--platform` and
`--config` are always required and should precede <mode>.
 
* AutoTuner: `python3 distributed.py tune -h`
    
Example:
  
```shell
python3 distributed.py --design gcd --platform sky130hd \
                       --config ../designs/sky130hd/gcd/autotuner.json \
                       tune
```
    
 
* Parameter sweeping: `python3 distributed.py sweep -h`
  
Example:
  
```shell
python3 distributed.py --design gcd --platform sky130hd \
                       --config distributed-sweep-example.json \
                       sweep
```


### Google Cloud Platform (GCP) distribution with Ray.

GCP Setup Tutorial coming soon.


### List of input arguments
    
* Target design
    - --design
        - Name of the design for autotuning
    - --platform
        - Name of the platform for autotuning
* Experiment setup
    - --config
        - Configuration file that sets which knobs to use for autotuning
    - --experiment
        - Experiment name. This parameter is used to prefix the `FLOW_VARIANT` and to set the Ray log destination
    - --resume
        - Resume previous run
* Git setup
    - --git-clean
        - Clean binaries and build files 
    - --git-clone
        - Force new git clone
    - --git-clone-args
        - Additional git clone arguments
    - --git-latest
        - Use the latest version of OR app
    - --git-or-branch
        - OR app branch to use
    - --git-orfs-branch
        - ORFS branch to use
    - --git-url
        - ORFS repo URL to use
    - --build-args
        - Additional arguments given to `./build_openroad.sh`
* For AutoTuner
    - --algorithm
        - Search algorithm to use for autotuning
    - --eval
        - Evaluate function to use with search algorithm
    - --samples
        - Number of samples for autotuning
    - --iterations
        - Number of iterations for autotuning
    - --reference
        - Reference file for use with "PPAImprov" evaluation function
    - --perturbation
        - Perturbation interval for PopulationBasedTraining
    - --seed
        - Random seed for parameter selection during autotuning
* Workload
    - --jobs
        - Max number of concurrent jobs
    - --openroad-threads
        - Max number of threads OR app can use
    - --server
        - The address of Ray server to connect
    - --port
        - The port of Ray server to connect
    - -v, --verbose
        - Verbosity level
            - 0: only print Ray status
            - 1: also print training stderr
            - 2: also print training stdout

    
### GUI 

Basically, progress is displayed at the terminal where you run, and when all runs are finished, the results are displayed. 
You could find the "Best config found" on the screen.

To use TensorBoard GUI, run `tensorboard --logdir=./<logpath>`. While TensorBoard is running, you can open the webpage http://localhost:6006/ to see the GUI.
    
## Citation

Please cite the following paper.

* J. Jung, A. B. Kahng, S. Kim and R. Varadarajan, "METRICS2.1 and Flow Tuning in the IEEE CEDA Robust Design Flow and OpenROAD", [(.pdf)](https://vlsicad.ucsd.edu/Publications/Conferences/388/c388.pdf), [(.pptx)](https://vlsicad.ucsd.edu/Publications/Conferences/388/c388.pptx), [(.mp4)](https://vlsicad.ucsd.edu/Publications/Conferences/388/c388.mp4), Proc. ACM/IEEE International Conference on Computer-Aided Design, 2021.

## Acknowledgments
   
AutoTuner has been developed by UCSD with the OpenROAD Project.
