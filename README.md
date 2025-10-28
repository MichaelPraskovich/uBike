# Universal Bike Controller
This repository is a fork of [nategreco/uBike](https://github.com/nategreco/uBike), an open-source drop-in replacement for the NordicTrack S-series studio cycle consoles.

The changes in this fork include an **LLM-generated, physics-based power calculation** for estimating watts. The motivation was based on an observation that resistance levels **2-8 consistently report only 1 W**, raising concerns that other resistance levels may misestimate power output.

## Measurements Taken
Flywheel Diameter: 455.65 mm
Flywheel Pulley Diameter: 58.3 mm
Crank Pulley Diameter: 310 mm
Crank -> Flywheel Ratio: ~5.317 : 1

## Assumptions Made
- The flywheel mass was provided at **14.515 kg (32 lb)** -- the advertised weight cited by NordicTrack and several independent reviews.
- Mechanical efficiency figure was estimated at 0.95.

## Coast-Down Experiment Data
Anchor values for the model were computed from several **60 rpm → 0 rpm coast down experiments**.

The results of these experiments are provided below. 

**Note:** This data was gathered by filming the uBike's console RPM readout. I may refine these measurements later as determining the precise zero-RPM timestamp from the footage introduced minor uncertainty.

<details> <summary><b>Resistance 1</b></summary>

| **time_s** | **rpm (run 1)** | **rpm (run 2)** |
| :--------: | :-------------: | :-------------: |
|     0.0    |        60       |        59       |
|     0.5    |        59       |                 |
|     1.0    |                 |        58       |
|     1.1    |        58       |                 |
|     1.5    |        56       |        55       |
|     2.0    |                 |        52       |
|     2.1    |        53       |                 |
|     2.5    |                 |        49       |
|     3.1    |        49       |                 |
|     3.5    |        46       |        45       |
|     4.5    |        42       |        41       |
|     5.5    |        38       |        37       |
|     6.5    |        34       |        33       |
|     7.5    |                 |        29       |
|     8.0    |        30       |                 |
|     9.5    |        25       |        25       |
|    11.5    |        0        |        0        |

<details> <summary><b>Resistance 4</b></summary>

| **time_s** | **rpm (run 1)** | **rpm (run 2)** |
| :--------: | :-------------: | :-------------: |
|     0.0    |        60       |        60       |
|     0.5    |        59       |        59       |
|     1.0    |        58       |                 |
|     1.5    |                 |        57       |
|     2.0    |        57       |        55       |
|     2.5    |        54       |        51       |
|     3.0    |        51       |                 |
|     3.5    |                 |        47       |
|     4.0    |        48       |        44       |
|     4.5    |        44       |                 |
|     5.0    |                 |        40       |
|     5.5    |        40       |                 |
|     6.0    |                 |        36       |
|     6.5    |        36       |                 |
|     7.0    |                 |        31       |
|     7.5    |        32       |                 |
|     8.5    |                 |        27       |
|     9.0    |        27       |                 |
|    10.5    |                 |        22       |
|    11.0    |        23       |                 |
|    12.5    |                 |        0        |
|    13.0    |        0        |                 |

<details> <summary><b>Resistance 10</b></summary>

| **time_s** | **rpm (run 1)** | **rpm (run 2)** |
| :--------: | :-------------: | :-------------: |
|     0.0    |        61       |        60       |
|     0.1    |                 |        59       |
|     0.3    |        60       |                 |
|     0.8    |        59       |                 |
|     1.0    |                 |        57       |
|     1.3    |        56       |                 |
|     2.0    |                 |        54       |
|     2.3    |        51       |                 |
|     2.5    |                 |        49       |
|     2.8    |        44       |                 |
|     3.5    |                 |        42       |
|     4.3    |        36       |                 |
|     5.0    |                 |        34       |
|     6.3    |        28       |                 |
|     7.0    |                 |        26       |
|     7.8    |        0        |                 |
|     9.0    |                 |        0        |

<details> <summary><b>Resistance 13</b></summary>

| **time_s** | **rpm (run 1)** | **rpm (run 2)** |
| :--------: | :-------------: | :-------------: |
|     0.0    |        60       |        59       |
|     0.2    |                 |        57       |
|     0.7    |                 |        54       |
|     1.0    |        58       |                 |
|     1.5    |        52       |                 |
|     1.7    |                 |        47       |
|     3.0    |        43       |                 |
|     3.2    |                 |        38       |
|     5.0    |        32       |                 |
|     5.2    |                 |        0        |
|     7.0    |        0        |                 |

<details> <summary><b>Resistance 16</b></summary>

| **time_s** | **rpm (run 1)** | **rpm (run 2)** |
| :--------: | :-------------: | :-------------: |
|     0.0    |        61       |        59       |
|     0.1    |                 |        58       |
|     0.5    |        59       |        56       |
|     1.5    |        52       |        50       |
|     3.0    |        41       |        40       |
|     5.0    |        0        |        0        |

<details> <summary><b>Resistance 18</b></summary>

| **time_s** | **rpm (run 1)** | **rpm (run 2)** |
| :--------: | :-------------: | :-------------: |
|     0.0    |        61       |        61       |
|     0.1    |                 |        59       |
|     0.9    |        59       |                 |
|     1.1    |                 |        51       |
|     1.9    |        52       |                 |
|     3.1    |                 |        0        |
|     3.4    |        0        |                 |

<details> <summary><b>Resistance 20</b></summary>

| **time_s** | **rpm (run 1)** | **rpm (run 2)** |
| :--------: | :-------------: | :-------------: |
|     0.0    |        60       |        60       |
|     0.5    |        58       |                 |
|     0.7    |                 |        56       |
|     1.5    |        48       |                 |
|     2.3    |                 |        46       |
|     3.5    |        0        |                 |
|     3.7    |                 |        0        |

<details> <summary><b>Resistance 22</b></summary>

| **time_s** | **rpm (run 1)** | **rpm (run 2)** |
| :--------: | :-------------: | :-------------: |
|     0.0    |        60       |        59       |
|     0.5    |                 |        55       |
|     0.6    |        56       |                 |
|     2.0    |                 |        0        |
|     2.6    |        0        |                 |
