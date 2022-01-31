find_path(BROADCOM_INCLUDE_DIR
  NAMES bcm_host.h
  DOC "Broadcom include directory"
  PATHS /opt/vc/include)
mark_as_advanced(BCM_INCLUDE_DIR)

find_library(VCOS_LIBRARY
  NAMES libvcos.so
  DOC "Path to VCOS Library"
  PATHS /opt/vc/lib)
mark_as_advanced(VCOS_LIBRARY)

find_library(VCHIQ_LIBRARY
  NAMES libvchiq_arm.so
  DOC "Path to VCHIQ Library"
  PATHS /opt/vc/lib)
mark_as_advanced(VCHIQ_LIBRARY)

find_library(OPENMAXIL_LIBRARY
  NAMES libopenmaxil.so
  DOC "Path to OpenMAX IL Library"
  PATHS /opt/vc/lib)
mark_as_advanced(OPENMAXIL_LIBRARY)

find_library(BCM_HOST_LIBRARY
  NAMES libbcm_host.so
  DOC "Path to Broadcom Host Library"
  PATHS /opt/vc/lib)
mark_as_advanced(BCM_HOST_LIBRARY)

find_library(MMAL_CORE_LIBRARY
  NAMES libmmal_core.so
  DOC "Path to MMAL Core Library"
  PATHS /opt/vc/lib)
mark_as_advanced(MMAL_CORE_LIBRARY)

find_library(MMAL_UTIL_LIBRARY
  NAMES libmmal_util.so
  DOC "Path to MMAL Util Library"
  PATHS /opt/vc/lib)
mark_as_advanced(MMAL_UTIL_LIBRARY)

find_library(MMAL_VC_CLIENT_LIBRARY
  NAMES libmmal_vc_client.so
  DOC "Path to MMAL Client Library"
  PATHS /opt/vc/lib)
mark_as_advanced(MMAL_VC_CLIENT_LIBRARY)

include(${CMAKE_ROOT}/Modules/FindPackageHandleStandardArgs.cmake)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(Broadcom DEFAULT_MSG BROADCOM_INCLUDE_DIR VCOS_LIBRARY VCHIQ_LIBRARY OPENMAXIL_LIBRARY BCM_HOST_LIBRARY)

set(BROADCOM_OMX_LIBRARIES ${BCM_HOST_LIBRARY} ${OPENMAXIL_LIBRARY} ${VCHIQ_LIBRARY} ${VCOS_LIBRARY})
set(BROADCOM_MMAL_LIBRARIES ${BCM_HOST_LIBRARY} ${VCOS_LIBRARY} ${MMAL_CORE_LIBRARY} ${MMAL_UTIL_LIBRARY} ${MMAL_VC_CLIENT_LIBRARY})
set(BROADCOM_INCLUDE_DIRS ${BROADCOM_INCLUDE_DIR} ${BROADCOM_INCLUDE_DIR}/interface/vmcs_host/linux ${BROADCOM_INCLUDE_DIR}/interface/vcos/pthreads)
set(BROADCOM_OMX_DEFINITIONS USE_VCHIQ_ARM HAVE_LIBOPENMAX=2 OMX OMX_SKIP64BIT USE_EXTERNAL_OMX HAVE_LIBBCM_HOST USE_EXTERNAL_LIBBCM_HOST)
