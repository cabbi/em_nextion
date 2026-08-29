# 1.0.0
# 1.0.1
# 1.0.2
- first releases with several adjustments and fixes

# 1.0.3
- New templated 'Get' and 'Set' methods to avoid conversions

# 1.0.4
- Removed 'timeoutMs' default value from 'EmNextion' constructor since its strongly dependant from serial baud rate              
- Added 'setVisible' methods to set objects visibility
- Added Set & Get Picture
- Added Set & Get Background and Font color
- Added Click method
- Added "dummy" example
- Added 'TODO.md' file

# 1.0.5
- Adjusted dependency on EmCore version 1.0.5
  
# 1.0.6
- Reduced RAM footprint by templating the page and splitting 'EmValue' derived classes 

# 1.0.7
- BUG fix in '_getNumber' method 

# 2.0.0
- Aligned to new EmCore 2.0.0 code restyling
- Added 'EmNexCfgInteger' to handle configuration parameters that need to be set/initialized at startup

# 2.1.0
- Added 'EmNexIntegerTag', 'EmNexRealTag' and 'EmNexDecimalTag' to support a persistent value in the 'EmTags' class
- Added 'EmNexIntegerTag', 'EmNexRealTag' and 'EmNexDecimalTag' to support a persistent value in the 'EmTags' class
- Added 'scanBaudrate' and 'wakup' methods.
- New 'EmSerialStream' class to handle serial stream directly in the nextion object (NOTE: this change break backwards compatibility!)
- Typo fixes