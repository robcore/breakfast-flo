ccflags-y := -Ofast -fgcse-las -fpredictive-commoning \
	     -ffast-math -fgcse-lm -fgcse-sm -fsched-spec-load \
	     -fforce-addr -fsingle-precision-constant -mcpu=cortex-a9 \
	     -mtune=cortex-a9 -marm -mfpu=neon-vfpv4 -fgraphite -fgraphite-identity \
	     -floop-flatten -floop-parallelize-all -ftree-loop-linear \
	     -floop-interchange -floop-strip-mine -floop-block

obj-$(CONFIG_ION) +=	ion.o ion_heap.o ion_system_heap.o ion_carveout_heap.o ion_iommu_heap.o ion_cp_heap.o
obj-$(CONFIG_CMA) += ion_cma_heap.o
obj-$(CONFIG_ION_TEGRA) += tegra/
obj-$(CONFIG_ION_MSM) += msm/
