# Change Log

## v 1.2.0
* adopt Arduino 3.x core builds
* remove 3rd party LinkedList lib dependency

## v 1.1.1 (2023-12-09)
* update examples with more detailed callback code
+ example on how to create and use TimeSeries data collector
* code style formatting
+ Averaging function for TimeSeries container

## v 1.1.0 (2023-08-21)
+ add DummyPZEM004 device with emulated metrics 
+ add NullQ and NullCable objects
* trivial virtual destructors were missing body
* minor code refactoring

## v 1.0.3 (2022-06-03)
 + added  PZEM::resetEnergyCounter(), PZPool::resetEnergyCounter() methods
 * fix: PZPool::addPZEM() method unable to create new PZEM objects in the pool
 * some code and examples refactoring

## v 1.0.2 (2022-04-06)
 * fix stale data calculation
 * fix IDF build with timeseries feature for IDF 3/4 and arduino core 1.x and 2.x 

## v 1.0.1 (2022-01-30)
 - fix incorrect byte shift for 4-byte values
## v 1.0.0 (2021-12-28)
 - Abstracted Message Queue class
 - Add TimeSeries feature
   - RingBuffer class for storing data structs (support mem allocation in PSRAM)
   - Iterator class to traverse ring buffers
   - TimeSeries object and container for custom sets of TS data
 - a pool class could manage mixed pzem device types simultaneously
 - support for PZEM003
 - building under esp-idf
 - pzem_cli - added 'energy reset', 'alarm get/set' features
 - examples for Sigle/Multiple PZEMs
 - pzem_cli example
 - initial version