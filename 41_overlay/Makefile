# 所有 .dts 源文件
SRCS := $(wildcard *.dts)

# 分类：含 "overlay" 的 vs 不含的
OVERLAY_SRCS   := $(foreach s,$(SRCS),$(if $(findstring overlay,$(s)),$(s)))
REGULAR_SRCS   := $(filter-out $(OVERLAY_SRCS),$(SRCS))


# 对应的目标文件
OVERLAY_OBJS   := $(OVERLAY_SRCS:%.dts=%.dtbo)
REGULAR_OBJS   := $(REGULAR_SRCS:%.dts=%.dtb)

# 所有目标
OBJS := $(OVERLAY_OBJS) $(REGULAR_OBJS)

# 反编译目标：.dtbo -> -rev.dts, .dtb -> -rev.dts
# 先把dtb映射dts
REV_DTS := $(OBJS:%.dtb=%-rev.dts)
# 再把剩下的dtbo映射为dts
REV_DTS := $(REV_DTS:%.dtbo=%-rev.dts)

.PHONY: all clean rev

all: $(OBJS)

# 编译普通设备树：.dts → .dtb（不含 overlay）
%.dtb: %.dts
	dtc -I dts -O dtb $< -o $@

# 编译 overlay：.dts → .dtbo（含 overlay），加 -@ 保留符号
%.dtbo: %.dts
	dtc -I dts -O dtb -@ $< -o $@

# 反编译：无论是 .dtb 还是 .dtbo，都生成 %-rev.dts
%-rev.dts: %.dtb
	dtc -I dtb -O dts $< -o $@

%-rev.dts: %.dtbo
	dtc -I dtb -O dts $< -o $@

rev: $(REV_DTS)

clean:
	rm -f $(OBJS) $(REV_DTS)
