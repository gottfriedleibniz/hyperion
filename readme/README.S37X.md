![header image](images/image_header_herculeshyperionSDL.png)
[Return to master README.md](../README.md)

# S/370 Backport of select ESA/390 and z/Architecture instructions: the `HERCULES_370_EXTENSION` facility

## Contents

1. [About](#About)
2. [Enabling the additional instructions](#Enabling-the-additional-instructions)
3. [List of affected instructions](#List-of-affected-instructions)

# About

Some ESA/390 and z/Architecture features and their instructions are architecturally compatible with the S/370 architecture.  Although they are not present in the [S/370 Principle of Operations (GA22-7000)](http://www.bitsavers.org/pdf/ibm/370/princOps/GA22-7000-10_370_Principles_of_Operation_Sep87.pdf), they are not in contradiction with the reference manual. 

For example, there is no contradication for an instruction such as LHI (Load Halfword Immediate) to be included as part of the S/370 architecture presented by Hercules. 

However, since these instruction are not part of the original architecture, it is necessary that these extensions to the architecture be controlled at runtime. 

Originally, the fact that such and such facility or feature was built for such and such architecture could only be controlled by a series of C preprocessor macros in the [feat370.h](https://github.com/SDL-Hercules-390/hyperion/blob/Release_4.9/feat370.h), [feat390.h](https://github.com/SDL-Hercules-390/hyperion/blob/Release_4.9/feat390.h) and [feat900.h](https://github.com/SDL-Hercules-390/hyperion/blob/Release_4.9/feat900.h) header files. 

Furthermore, the availability of the instructions is controlled by Operation code tables in [opcode.c](https://github.com/SDL-Hercules-390/hyperion/blob/Release_4.9/opcode.c#L4173-L4211). 

Before runtime control was available, a [select number of features were made available in feat370.h](https://github.com/SDL-Hercules-390/hyperion/blob/Release_4.9/feat370.h#L80-L129) and then commented out.  Removing the comment and rebuilding Hercules then made it possible to access those features in the S/370 architectural mode. 

However, requiring a rebuild seemed a little too much to ask of the casual Hercules user since it would mean they would have to manually build a custom version of Hercules for themselves, which is not something a casual user of Hercules is necessarily prepared to do. 

## Enabling the additional instructions

From the configuration file or hardware control panel, simply issue the commands: 

<pre>
    archlvl   370
    facility  enable  herc_370_extension      # Original implementation
    <i>facility  enable  herc_370_extension_2    # (future?)
    facility  enable  herc_370_extension_3    # (future?)</i>
                ...etc...
</pre>

This will enable _(or disable if_ `'disable'` _is specified instead)_ the currently defined backported instructions to then be available _(or unavailable)_ in System/370 mode. 

## List of affected instructions

For the current list of affected instructions, refer to the [`BEG_DIS_FAC_INS_FUNC( herc37X )`](https://github.com/SDL-Hercules-390/hyperion/blob/Release_4.9/facility.c#L4272-L4457) section of the [`facility.c`](https://github.com/SDL-Hercules-390/hyperion/blob/Release_4.9/facility.c) source file. 

&nbsp;
&nbsp;
&nbsp;
