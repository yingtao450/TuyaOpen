# 当前文件所在目录
LOCAL_PATH := $(call my-dir)
#---------------------------------------

# 清除 LOCAL_xxx 变量
include $(CLEAR_VARS)
#
LOCAL_PKG_NAME := $(notdir $(LOCAL_PATH))
# LOCAL_PKG_SRC := $(abspath $(LOCAL_PATH))
LOCAL_PKG_SRC :=
LOCAL_PKG_DEPS :=
LOCAL_PKG_EXPORT :=
LOCAL_PKG_OPTS := --disable-protoc

# 模块的 CFLAGS
ifeq (,$(findstring -mslow-flash-data,$(TUYA_PLATFORM_CFLAGS)))
    $(info -mslow-flash-data not found in TUYA_PLATFORM_CFLAGS)
	LOCAL_CFLAGS := -fPIC
else
    $(info -mslow-flash-data found in TUYA_PLATFORM_CFLAGS)
	LOCAL_CFLAGS :=
endif

ifneq ($(CONFIG_OPERATING_SYSTEM),100)
    LOCAL_CFLAGS+=-DNDEBUG
endif
LOCAL_CFLAGS += $(TUYA_PLATFORM_CFLAGS)

exist = $(shell if [ ! -f ${LOCAL_PATH}/configure ]; then cd ${LOCAL_PATH}; autoreconf -f -i; else echo exist;fi;)
ifneq ($(exist), "exist")
$(info "create configure")
endif

ifeq ($(CONFIG_ENABLE_BUILD_PROTOBUF_C), y)
include $(BUILD_PACKAGE)
LOCAL_PROTOBUF_C_PATH := $(call static-objects-dir)/$(LOCAL_PATH)
protobuf_c_static:
	@echo "====== protobuf_c ar -x ======="
	@if [ -f $(OUTPUT_DIR)/lib/libprotobuf-c.a ]; then \
		echo $(LOCAL_PROTOBUF_C_PATH); \
		mkdir -p $(LOCAL_PROTOBUF_C_PATH); \
		cp $(OUTPUT_DIR)/lib/libprotobuf-c.a $(LOCAL_PROTOBUF_C_PATH); \
		cd $(LOCAL_PROTOBUF_C_PATH); \
		$(AR) -x libprotobuf-c.a; \
		rm libprotobuf-c.a; fi
	@echo "------ protobuf_c ar -x end ---------"


protobuf_c_static: $(_LOCAL_PKG_TARGET_BUILD)
os_sub_static: protobuf_c_static

endif
