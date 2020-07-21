# Plot helpers

In here you will find a few Python scripts that can be used to easily plot certain parts of a basic simulation.

## Requirements

* A recent version of gnuplot: `sudo apt-get install gnuplot`
* exputil: `pip install git+https://github.com/snkas/exputilpy.git`

## Getting started

Suppose you have your `logs_ns3` folder at `path/to/run_folder/logs_ns3`

### Flow plotting

Plotting flow with id 0 (cwnd, progress, RTT, rate (in 10ms intervals)):

```
cd /path/to/plot_helpers/flow_plot
python flow_plot.py path/to/run_folder/logs_ns3 path/to/run_folder/logs_ns3/data path/to/run_folder/logs_ns3/pdf 0 10000000
```

Generating only the rate data file for flow id 0 (in a 10ms interval):

```
cd /path/to/plot_helpers/flow_plot
python generate_flow_rase_csv.py path/to/run_folder/logs_ns3 path/to/run_folder/logs_ns3/data 0 10000000
```


### Ping plotting

Plotting pings between 21 -> 30 (RTT, # of out-of-order (in 10ms intervals))

```
cd /path/to/plot_helpers/ping_plot
python ping_plot.py path/to/run_folder/logs_ns3 path/to/run_folder/logs_ns3/data path/to/run_folder/logs_ns3/pdf 21 30 10000000
```


### Link utilization plotting

Plotting utilization of link 5 -> 9:
```
cd /path/to/plot_helpers/utilization_plot
python utilization_plot.py path/to/run_folder/logs_ns3 path/to/run_folder/logs_ns3/data path/to/run_folder/logs_ns3/pdf 5 9
```
