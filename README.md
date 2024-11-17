# em_nextion
C++ library for embedded Nextion display communication

The aim of this library is to provide a set of tiny RAM and Flash footprint classes.
Templates are used to limit RAM consumption at minimum.

NOTE: 
To limit RAM footprint some classes (e.g. EmNexText) are split to "non virtual" implementation and '<class_name>Ex' classes (e.g. EmNexTextEx) overriding 'GetValue' and 'SetValue' of 'EmValue' class.