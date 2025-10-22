################################################################################
# Automatically-generated file. Do not edit!
################################################################################

SHELL = cmd.exe

# Each subdirectory must supply rules for building sources it contributes
%.obj: ../%.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: C2000 Compiler'
	"C:/ti/ccs2020/ccs/tools/compiler/ti-cgt-c2000_22.6.2.LTS/bin/cl2000" -v28 -ml -mt --cla_support=cla2 --float_support=fpu32 --tmu_support=tmu0 --vcu_support=vcu0 -Ooff --include_path="C:/Users/13281/workspace_ccstheia/freertos_demo_280049" --include_path="C:/ti/C2000Ware_6_00_00_00" --include_path="C:/Users/13281/workspace_ccstheia/freertos_demo_280049/device" --include_path="C:/ti/C2000Ware_6_00_00_00/driverlib/f28004x/driverlib" --include_path="C:/ti/C2000Ware_6_00_00_00/kernel/FreeRTOS/Source/portable/CCS/C2000_C28x" --include_path="C:/ti/C2000Ware_6_00_00_00/kernel/FreeRTOS/Source/include" --include_path="C:/ti/ccs2020/ccs/tools/compiler/ti-cgt-c2000_22.6.2.LTS/include" --define=DEBUG --define=RAM --diag_suppress=10063 --diag_warning=225 --diag_wrap=off --display_error_number --gen_func_subsections=on --abi=eabi --preproc_with_compile --preproc_dependency="$(basename $(<F)).d_raw" --include_path="C:/Users/13281/workspace_ccstheia/freertos_demo_280049/CPU1_RAM/syscfg" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: "$<"'
	@echo ' '

build-112129393: ../main.syscfg
	@echo 'Building file: "$<"'
	@echo 'Invoking: SysConfig'
	"C:/ti/ccs2020/ccs/utils/sysconfig_1.25.0/sysconfig_cli.bat" --script "C:/Users/13281/workspace_ccstheia/freertos_demo_280049/main.syscfg" -o "syscfg" -s "C:/ti/C2000Ware_6_00_00_00/.metadata/sdk.json" -d "F28004x" -p "F28004x_64PM" -r "F28004x_64PM" --context "system" --compiler ccs
	@echo 'Finished building: "$<"'
	@echo ' '

syscfg/board.c: build-112129393 ../main.syscfg
syscfg/board.h: build-112129393
syscfg/board.cmd.genlibs: build-112129393
syscfg/board.opt: build-112129393
syscfg/board.json: build-112129393
syscfg/pinmux.csv: build-112129393
syscfg/c2000ware_libraries.cmd.genlibs: build-112129393
syscfg/c2000ware_libraries.opt: build-112129393
syscfg/c2000ware_libraries.c: build-112129393
syscfg/c2000ware_libraries.h: build-112129393
syscfg/c2000_freertos.c: build-112129393
syscfg/c2000_freertos.h: build-112129393
syscfg/FreeRTOSConfig.h: build-112129393
syscfg/c2000_freertos.cmd.genlibs: build-112129393
syscfg/c2000_freertos.opt: build-112129393
syscfg/syscfg_c.rov.xs: build-112129393
syscfg/clocktree.h: build-112129393
syscfg: build-112129393

syscfg/%.obj: ./syscfg/%.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: C2000 Compiler'
	"C:/ti/ccs2020/ccs/tools/compiler/ti-cgt-c2000_22.6.2.LTS/bin/cl2000" -v28 -ml -mt --cla_support=cla2 --float_support=fpu32 --tmu_support=tmu0 --vcu_support=vcu0 -Ooff --include_path="C:/Users/13281/workspace_ccstheia/freertos_demo_280049" --include_path="C:/ti/C2000Ware_6_00_00_00" --include_path="C:/Users/13281/workspace_ccstheia/freertos_demo_280049/device" --include_path="C:/ti/C2000Ware_6_00_00_00/driverlib/f28004x/driverlib" --include_path="C:/ti/C2000Ware_6_00_00_00/kernel/FreeRTOS/Source/portable/CCS/C2000_C28x" --include_path="C:/ti/C2000Ware_6_00_00_00/kernel/FreeRTOS/Source/include" --include_path="C:/ti/ccs2020/ccs/tools/compiler/ti-cgt-c2000_22.6.2.LTS/include" --define=DEBUG --define=RAM --diag_suppress=10063 --diag_warning=225 --diag_wrap=off --display_error_number --gen_func_subsections=on --abi=eabi --preproc_with_compile --preproc_dependency="syscfg/$(basename $(<F)).d_raw" --include_path="C:/Users/13281/workspace_ccstheia/freertos_demo_280049/CPU1_RAM/syscfg" --obj_directory="syscfg" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: "$<"'
	@echo ' '

tasks.obj: C:/ti/C2000Ware_6_00_00_00/kernel/FreeRTOS/Source/tasks.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: C2000 Compiler'
	"C:/ti/ccs2020/ccs/tools/compiler/ti-cgt-c2000_22.6.2.LTS/bin/cl2000" -v28 -ml -mt --cla_support=cla2 --float_support=fpu32 --tmu_support=tmu0 --vcu_support=vcu0 -Ooff --include_path="C:/Users/13281/workspace_ccstheia/freertos_demo_280049" --include_path="C:/ti/C2000Ware_6_00_00_00" --include_path="C:/Users/13281/workspace_ccstheia/freertos_demo_280049/device" --include_path="C:/ti/C2000Ware_6_00_00_00/driverlib/f28004x/driverlib" --include_path="C:/ti/C2000Ware_6_00_00_00/kernel/FreeRTOS/Source/portable/CCS/C2000_C28x" --include_path="C:/ti/C2000Ware_6_00_00_00/kernel/FreeRTOS/Source/include" --include_path="C:/ti/ccs2020/ccs/tools/compiler/ti-cgt-c2000_22.6.2.LTS/include" --define=DEBUG --define=RAM --diag_suppress=10063 --diag_warning=225 --diag_wrap=off --display_error_number --gen_func_subsections=on --abi=eabi --preproc_with_compile --preproc_dependency="$(basename $(<F)).d_raw" --include_path="C:/Users/13281/workspace_ccstheia/freertos_demo_280049/CPU1_RAM/syscfg" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: "$<"'
	@echo ' '

queue.obj: C:/ti/C2000Ware_6_00_00_00/kernel/FreeRTOS/Source/queue.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: C2000 Compiler'
	"C:/ti/ccs2020/ccs/tools/compiler/ti-cgt-c2000_22.6.2.LTS/bin/cl2000" -v28 -ml -mt --cla_support=cla2 --float_support=fpu32 --tmu_support=tmu0 --vcu_support=vcu0 -Ooff --include_path="C:/Users/13281/workspace_ccstheia/freertos_demo_280049" --include_path="C:/ti/C2000Ware_6_00_00_00" --include_path="C:/Users/13281/workspace_ccstheia/freertos_demo_280049/device" --include_path="C:/ti/C2000Ware_6_00_00_00/driverlib/f28004x/driverlib" --include_path="C:/ti/C2000Ware_6_00_00_00/kernel/FreeRTOS/Source/portable/CCS/C2000_C28x" --include_path="C:/ti/C2000Ware_6_00_00_00/kernel/FreeRTOS/Source/include" --include_path="C:/ti/ccs2020/ccs/tools/compiler/ti-cgt-c2000_22.6.2.LTS/include" --define=DEBUG --define=RAM --diag_suppress=10063 --diag_warning=225 --diag_wrap=off --display_error_number --gen_func_subsections=on --abi=eabi --preproc_with_compile --preproc_dependency="$(basename $(<F)).d_raw" --include_path="C:/Users/13281/workspace_ccstheia/freertos_demo_280049/CPU1_RAM/syscfg" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: "$<"'
	@echo ' '

list.obj: C:/ti/C2000Ware_6_00_00_00/kernel/FreeRTOS/Source/list.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: C2000 Compiler'
	"C:/ti/ccs2020/ccs/tools/compiler/ti-cgt-c2000_22.6.2.LTS/bin/cl2000" -v28 -ml -mt --cla_support=cla2 --float_support=fpu32 --tmu_support=tmu0 --vcu_support=vcu0 -Ooff --include_path="C:/Users/13281/workspace_ccstheia/freertos_demo_280049" --include_path="C:/ti/C2000Ware_6_00_00_00" --include_path="C:/Users/13281/workspace_ccstheia/freertos_demo_280049/device" --include_path="C:/ti/C2000Ware_6_00_00_00/driverlib/f28004x/driverlib" --include_path="C:/ti/C2000Ware_6_00_00_00/kernel/FreeRTOS/Source/portable/CCS/C2000_C28x" --include_path="C:/ti/C2000Ware_6_00_00_00/kernel/FreeRTOS/Source/include" --include_path="C:/ti/ccs2020/ccs/tools/compiler/ti-cgt-c2000_22.6.2.LTS/include" --define=DEBUG --define=RAM --diag_suppress=10063 --diag_warning=225 --diag_wrap=off --display_error_number --gen_func_subsections=on --abi=eabi --preproc_with_compile --preproc_dependency="$(basename $(<F)).d_raw" --include_path="C:/Users/13281/workspace_ccstheia/freertos_demo_280049/CPU1_RAM/syscfg" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: "$<"'
	@echo ' '

timers.obj: C:/ti/C2000Ware_6_00_00_00/kernel/FreeRTOS/Source/timers.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: C2000 Compiler'
	"C:/ti/ccs2020/ccs/tools/compiler/ti-cgt-c2000_22.6.2.LTS/bin/cl2000" -v28 -ml -mt --cla_support=cla2 --float_support=fpu32 --tmu_support=tmu0 --vcu_support=vcu0 -Ooff --include_path="C:/Users/13281/workspace_ccstheia/freertos_demo_280049" --include_path="C:/ti/C2000Ware_6_00_00_00" --include_path="C:/Users/13281/workspace_ccstheia/freertos_demo_280049/device" --include_path="C:/ti/C2000Ware_6_00_00_00/driverlib/f28004x/driverlib" --include_path="C:/ti/C2000Ware_6_00_00_00/kernel/FreeRTOS/Source/portable/CCS/C2000_C28x" --include_path="C:/ti/C2000Ware_6_00_00_00/kernel/FreeRTOS/Source/include" --include_path="C:/ti/ccs2020/ccs/tools/compiler/ti-cgt-c2000_22.6.2.LTS/include" --define=DEBUG --define=RAM --diag_suppress=10063 --diag_warning=225 --diag_wrap=off --display_error_number --gen_func_subsections=on --abi=eabi --preproc_with_compile --preproc_dependency="$(basename $(<F)).d_raw" --include_path="C:/Users/13281/workspace_ccstheia/freertos_demo_280049/CPU1_RAM/syscfg" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: "$<"'
	@echo ' '

event_groups.obj: C:/ti/C2000Ware_6_00_00_00/kernel/FreeRTOS/Source/event_groups.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: C2000 Compiler'
	"C:/ti/ccs2020/ccs/tools/compiler/ti-cgt-c2000_22.6.2.LTS/bin/cl2000" -v28 -ml -mt --cla_support=cla2 --float_support=fpu32 --tmu_support=tmu0 --vcu_support=vcu0 -Ooff --include_path="C:/Users/13281/workspace_ccstheia/freertos_demo_280049" --include_path="C:/ti/C2000Ware_6_00_00_00" --include_path="C:/Users/13281/workspace_ccstheia/freertos_demo_280049/device" --include_path="C:/ti/C2000Ware_6_00_00_00/driverlib/f28004x/driverlib" --include_path="C:/ti/C2000Ware_6_00_00_00/kernel/FreeRTOS/Source/portable/CCS/C2000_C28x" --include_path="C:/ti/C2000Ware_6_00_00_00/kernel/FreeRTOS/Source/include" --include_path="C:/ti/ccs2020/ccs/tools/compiler/ti-cgt-c2000_22.6.2.LTS/include" --define=DEBUG --define=RAM --diag_suppress=10063 --diag_warning=225 --diag_wrap=off --display_error_number --gen_func_subsections=on --abi=eabi --preproc_with_compile --preproc_dependency="$(basename $(<F)).d_raw" --include_path="C:/Users/13281/workspace_ccstheia/freertos_demo_280049/CPU1_RAM/syscfg" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: "$<"'
	@echo ' '

stream_buffer.obj: C:/ti/C2000Ware_6_00_00_00/kernel/FreeRTOS/Source/stream_buffer.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: C2000 Compiler'
	"C:/ti/ccs2020/ccs/tools/compiler/ti-cgt-c2000_22.6.2.LTS/bin/cl2000" -v28 -ml -mt --cla_support=cla2 --float_support=fpu32 --tmu_support=tmu0 --vcu_support=vcu0 -Ooff --include_path="C:/Users/13281/workspace_ccstheia/freertos_demo_280049" --include_path="C:/ti/C2000Ware_6_00_00_00" --include_path="C:/Users/13281/workspace_ccstheia/freertos_demo_280049/device" --include_path="C:/ti/C2000Ware_6_00_00_00/driverlib/f28004x/driverlib" --include_path="C:/ti/C2000Ware_6_00_00_00/kernel/FreeRTOS/Source/portable/CCS/C2000_C28x" --include_path="C:/ti/C2000Ware_6_00_00_00/kernel/FreeRTOS/Source/include" --include_path="C:/ti/ccs2020/ccs/tools/compiler/ti-cgt-c2000_22.6.2.LTS/include" --define=DEBUG --define=RAM --diag_suppress=10063 --diag_warning=225 --diag_wrap=off --display_error_number --gen_func_subsections=on --abi=eabi --preproc_with_compile --preproc_dependency="$(basename $(<F)).d_raw" --include_path="C:/Users/13281/workspace_ccstheia/freertos_demo_280049/CPU1_RAM/syscfg" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: "$<"'
	@echo ' '

port.obj: C:/ti/C2000Ware_6_00_00_00/kernel/FreeRTOS/Source/portable/CCS/C2000_C28x/port.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: C2000 Compiler'
	"C:/ti/ccs2020/ccs/tools/compiler/ti-cgt-c2000_22.6.2.LTS/bin/cl2000" -v28 -ml -mt --cla_support=cla2 --float_support=fpu32 --tmu_support=tmu0 --vcu_support=vcu0 -Ooff --include_path="C:/Users/13281/workspace_ccstheia/freertos_demo_280049" --include_path="C:/ti/C2000Ware_6_00_00_00" --include_path="C:/Users/13281/workspace_ccstheia/freertos_demo_280049/device" --include_path="C:/ti/C2000Ware_6_00_00_00/driverlib/f28004x/driverlib" --include_path="C:/ti/C2000Ware_6_00_00_00/kernel/FreeRTOS/Source/portable/CCS/C2000_C28x" --include_path="C:/ti/C2000Ware_6_00_00_00/kernel/FreeRTOS/Source/include" --include_path="C:/ti/ccs2020/ccs/tools/compiler/ti-cgt-c2000_22.6.2.LTS/include" --define=DEBUG --define=RAM --diag_suppress=10063 --diag_warning=225 --diag_wrap=off --display_error_number --gen_func_subsections=on --abi=eabi --preproc_with_compile --preproc_dependency="$(basename $(<F)).d_raw" --include_path="C:/Users/13281/workspace_ccstheia/freertos_demo_280049/CPU1_RAM/syscfg" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: "$<"'
	@echo ' '

portasm.obj: C:/ti/C2000Ware_6_00_00_00/kernel/FreeRTOS/Source/portable/CCS/C2000_C28x/portasm.asm $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: C2000 Compiler'
	"C:/ti/ccs2020/ccs/tools/compiler/ti-cgt-c2000_22.6.2.LTS/bin/cl2000" -v28 -ml -mt --cla_support=cla2 --float_support=fpu32 --tmu_support=tmu0 --vcu_support=vcu0 -Ooff --include_path="C:/Users/13281/workspace_ccstheia/freertos_demo_280049" --include_path="C:/ti/C2000Ware_6_00_00_00" --include_path="C:/Users/13281/workspace_ccstheia/freertos_demo_280049/device" --include_path="C:/ti/C2000Ware_6_00_00_00/driverlib/f28004x/driverlib" --include_path="C:/ti/C2000Ware_6_00_00_00/kernel/FreeRTOS/Source/portable/CCS/C2000_C28x" --include_path="C:/ti/C2000Ware_6_00_00_00/kernel/FreeRTOS/Source/include" --include_path="C:/ti/ccs2020/ccs/tools/compiler/ti-cgt-c2000_22.6.2.LTS/include" --define=DEBUG --define=RAM --diag_suppress=10063 --diag_warning=225 --diag_wrap=off --display_error_number --gen_func_subsections=on --abi=eabi --preproc_with_compile --preproc_dependency="$(basename $(<F)).d_raw" --include_path="C:/Users/13281/workspace_ccstheia/freertos_demo_280049/CPU1_RAM/syscfg" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: "$<"'
	@echo ' '

heap_4.obj: C:/ti/C2000Ware_6_00_00_00/kernel/FreeRTOS/Source/portable/MemMang/heap_4.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: C2000 Compiler'
	"C:/ti/ccs2020/ccs/tools/compiler/ti-cgt-c2000_22.6.2.LTS/bin/cl2000" -v28 -ml -mt --cla_support=cla2 --float_support=fpu32 --tmu_support=tmu0 --vcu_support=vcu0 -Ooff --include_path="C:/Users/13281/workspace_ccstheia/freertos_demo_280049" --include_path="C:/ti/C2000Ware_6_00_00_00" --include_path="C:/Users/13281/workspace_ccstheia/freertos_demo_280049/device" --include_path="C:/ti/C2000Ware_6_00_00_00/driverlib/f28004x/driverlib" --include_path="C:/ti/C2000Ware_6_00_00_00/kernel/FreeRTOS/Source/portable/CCS/C2000_C28x" --include_path="C:/ti/C2000Ware_6_00_00_00/kernel/FreeRTOS/Source/include" --include_path="C:/ti/ccs2020/ccs/tools/compiler/ti-cgt-c2000_22.6.2.LTS/include" --define=DEBUG --define=RAM --diag_suppress=10063 --diag_warning=225 --diag_wrap=off --display_error_number --gen_func_subsections=on --abi=eabi --preproc_with_compile --preproc_dependency="$(basename $(<F)).d_raw" --include_path="C:/Users/13281/workspace_ccstheia/freertos_demo_280049/CPU1_RAM/syscfg" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: "$<"'
	@echo ' '


