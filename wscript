# -*- Mode: python; py-indent-offset: 4; indent-tabs-mode: nil; coding: utf-8; -*-
import os
import shutil
import glob
import collections
from waflib import Context, Utils, Logs, Build

# 定义proto文件路径 - 修复__file__未定义的问题
# 使用Context.launch_dir或os.getcwd()来获取当前目录
PROTO_SRC_DIR = os.path.normpath(os.path.join(os.getcwd(), '..', '..', 'generated', 'cpp'))
PROTO_DST_DIR = os.path.join('helper', 'core')

def options(opt):
    pass

def configure(conf):
    # 在configure阶段更新路径信息
    global PROTO_SRC_DIR
    # 更新到正确的根目录 (假设ns3-sat-sim/simulator是根目录)
    PROTO_SRC_DIR = os.path.normpath(os.path.join(conf.path.abspath(), '..', '..', '..', 'generated', 'cpp'))
    
    # 检查源目录是否存在
    if not os.path.exists(PROTO_SRC_DIR):
        Logs.warn('Proto source directory does not exist: ' + PROTO_SRC_DIR)
        Logs.warn('Please ensure the proto files are generated first')
    else:
        Logs.info('Found proto source directory: ' + PROTO_SRC_DIR)
    
    # 检查protobuf和grpc库是否存在
    conf.check_cfg(package='protobuf', args=['--cflags', '--libs'], uselib_store='PROTOBUF')
    conf.check_cfg(package='grpc++', args=['--cflags', '--libs'], uselib_store='GRPC')
    
    if not conf.env.HAVE_PROTOBUF:
        conf.fatal('Google Protobuf library not found. Please install libprotobuf-dev')
    
    if not conf.env.HAVE_GRPC:
        conf.fatal('gRPC library not found. Please install libgrpc++-dev')

def pre_build(bld):
    """复制proto文件到目标目录"""
    # 确保使用正确的路径
    src_dir = PROTO_SRC_DIR
    Logs.info("Using proto source directory: %s" % src_dir)
    
    if not os.path.exists(src_dir):
        Logs.warn("Proto source directory not found: %s" % src_dir)
        return
    
    # 确保目标目录存在
    dst_dir = os.path.join(bld.path.abspath(), PROTO_DST_DIR)
    if not os.path.exists(dst_dir):
        os.makedirs(dst_dir, exist_ok=True)
    
    # 查找所有proto生成的文件，确保每种类型的文件只包含一次
    proto_cc_files = collections.OrderedDict()  # 使用OrderedDict去重
    proto_h_files = collections.OrderedDict()
    
    # 收集所有.pb.cc文件
    for pattern in ['*.pb.cc', '*.grpc.pb.cc']:
        for src_file in glob.glob(os.path.join(src_dir, pattern)):
            file_name = os.path.basename(src_file)
            if file_name not in proto_cc_files:
                proto_cc_files[file_name] = src_file
    
    # 收集所有.pb.h文件
    for pattern in ['*.pb.h', '*.grpc.pb.h']:
        for src_file in glob.glob(os.path.join(src_dir, pattern)):
            file_name = os.path.basename(src_file)
            if file_name not in proto_h_files:
                proto_h_files[file_name] = src_file
    
    if not proto_cc_files and not proto_h_files:
        Logs.warn("No proto files found in %s" % src_dir)
        return
    
    Logs.info("Found %d unique proto source files and %d unique proto header files to copy" % 
              (len(proto_cc_files), len(proto_h_files)))
    
    # 复制所有文件
    copied_files = []
    
    # 复制源文件
    for file_name, src_file in proto_cc_files.items():
        dst_file = os.path.join(dst_dir, file_name)
        if os.path.exists(dst_file):
            # 如果目标文件已存在，检查是否需要更新
            if os.path.getmtime(src_file) > os.path.getmtime(dst_file):
                Logs.info("Updating %s" % file_name)
                shutil.copy2(src_file, dst_file)
        else:
            Logs.info("Copying %s" % file_name)
            shutil.copy2(src_file, dst_file)
        copied_files.append(file_name)
    
    # 复制头文件
    for file_name, src_file in proto_h_files.items():
        dst_file = os.path.join(dst_dir, file_name)
        if os.path.exists(dst_file):
            # 如果目标文件已存在，检查是否需要更新
            if os.path.getmtime(src_file) > os.path.getmtime(dst_file):
                Logs.info("Updating %s" % file_name)
                shutil.copy2(src_file, dst_file)
        else:
            Logs.info("Copying %s" % file_name)
            shutil.copy2(src_file, dst_file)
        copied_files.append(file_name)
    
    # 返回复制的文件名，以便build函数使用
    return copied_files

def build(bld):
    # 在编译前复制proto文件，获取所有复制的文件名列表
    copied_files = []
    if not hasattr(Build, 'pre_build_called'):
        copied_files = pre_build(bld) or []
        Build.pre_build_called = True
    
    # 根据复制的文件筛选出源文件和头文件，确保不存在重复
    proto_src_files = []
    proto_header_files = []
    
    # 确保每个源文件只添加一次
    src_file_set = set()
    for file_name in copied_files:
        if file_name.endswith('.cc') and file_name not in src_file_set:
            proto_src_files.append(os.path.join(PROTO_DST_DIR, file_name))
            src_file_set.add(file_name)
    
    # 确保每个头文件只添加一次
    header_file_set = set()
    for file_name in copied_files:
        if file_name.endswith('.h') and file_name not in header_file_set:
            proto_header_files.append(os.path.join(PROTO_DST_DIR, file_name))
            header_file_set.add(file_name)
    
    Logs.info("Adding %d unique proto source files and %d unique proto header files to the build system" % 
              (len(proto_src_files), len(proto_header_files)))

    # Register 'basic-sim' module with dependencies
    # 添加protobuf和grpc库作为依赖
    module = bld.create_ns3_module('basic-sim', ['core', 'internet', 'applications', 'point-to-point', 'traffic-control'])
    
    # 添加外部库依赖
    module.use.extend(['PROTOBUF', 'GRPC'])
    
    # 添加编译选项
    module.cxxflags = ['-std=c++14']  # grpc和protobuf需要C++14
    
    # Source files
    base_source_files = [
        'model/core/message.cc',
        'helper/core/socket-helper.cc',
        'model/core/basic-simulation.cc',
        'model/core/exp-util.cc',
        'model/core/log-update-helper.cc',
        'model/core/topology-ptop.cc',
        'model/core/topology-ptop-queue-selector-default.cc',
        'model/core/topology-ptop-tc-qdisc-selector-default.cc',
        'model/core/arbiter.cc',
        'model/core/arbiter-ptop.cc',
        'model/core/arbiter-ecmp.cc',
        'model/core/ipv4-arbiter-routing.cc',
        'model/core/ptop-link-utilization-tracker.cc',
        'model/core/ptop-link-queue-tracker.cc',

        'helper/core/arbiter-ecmp-helper.cc',
        'helper/core/ipv4-arbiter-routing-helper.cc',
        'helper/core/ptop-link-utilization-tracker-helper.cc',
        'helper/core/ptop-link-queue-tracker-helper.cc',
        'helper/core/tcp-optimizer.cc',
        'helper/core/point-to-point-ab-helper.cc',

        'model/apps/tcp-flow-send-application.cc',
        'model/apps/tcp-flow-sink.cc',
        'model/apps/udp-rtt-client.cc',
        'model/apps/udp-burst-application.cc',
        'model/apps/udp-rtt-server.cc',
        'model/apps/udp-burst-info.cc',
        'model/apps/id-seq-header.cc',

        'helper/apps/tcp-flow-send-helper.cc',
        'helper/apps/tcp-flow-sink-helper.cc',
        'helper/apps/tcp-flow-schedule-reader.cc',
        'helper/apps/tcp-flow-scheduler.cc',
        'helper/apps/udp-rtt-helper.cc',
        'helper/apps/udp-burst-helper.cc',
        'helper/apps/udp-burst-schedule-reader.cc',
        'helper/apps/udp-burst-scheduler.cc',
        'helper/apps/pingmesh-scheduler.cc',
    ]
    
    # 添加proto生成的源文件
    module.source = base_source_files + proto_src_files

    # 基础头文件
    base_header_files = [
        'model/core/json.h',
        'model/core/message.h',
        'helper/core/socket-helper.h',
        'model/core/basic-simulation.h',
        'model/core/exp-util.h',
        'model/core/log-update-helper.h',
        'model/core/topology.h',
        'model/core/topology-ptop.h',
        'model/core/topology-ptop-queue-selector-default.h',
        'model/core/topology-ptop-tc-qdisc-selector-default.h',
        'model/core/arbiter.h',
        'model/core/arbiter-ptop.h',
        'model/core/arbiter-ecmp.h',
        'model/core/ipv4-arbiter-routing.h',
        'model/core/ptop-link-utilization-tracker.h',
        'model/core/ptop-link-queue-tracker.h',

        'helper/core/arbiter-ecmp-helper.h',
        'helper/core/ipv4-arbiter-routing-helper.h',
        'helper/core/ptop-link-utilization-tracker-helper.h',
        'helper/core/ptop-link-queue-tracker-helper.h',
        'helper/core/tcp-optimizer.h',
        'helper/core/point-to-point-ab-helper.h',

        'model/apps/tcp-flow-send-application.h',
        'model/apps/tcp-flow-sink.h',
        'model/apps/udp-rtt-client.h',
        'model/apps/udp-burst-application.h',
        'model/apps/udp-rtt-server.h',
        'model/apps/udp-burst-info.h',
        'model/apps/id-seq-header.h',

        'helper/apps/tcp-flow-send-helper.h',
        'helper/apps/tcp-flow-sink-helper.h',
        'helper/apps/tcp-flow-schedule-reader.h',
        'helper/apps/tcp-flow-scheduler.h',
        'helper/apps/udp-rtt-helper.h',
        'helper/apps/udp-burst-helper.h',
        'helper/apps/udp-burst-schedule-reader.h',
        'helper/apps/udp-burst-scheduler.h',
        'helper/apps/pingmesh-scheduler.h',
    ]

    # Header files
    headers = bld(features='ns3header')
    headers.module = 'basic-sim'
    # 添加proto生成的头文件
    headers.source = base_header_files + proto_header_files

    # Tests
    module_test = bld.create_ns3_module_test_library('basic-sim')
    module_test.source = [
        'test/basic-sim-test-suite.cc',
        ]
    
    # 测试也需要链接protobuf库
    module_test.use.extend(['PROTOBUF', 'GRPC'])

    # Main
    bld.recurse('main')

    # Examples
    if bld.env.ENABLE_EXAMPLES:
        bld.recurse('examples')

    # For now, no Python bindings are generated
    # bld.ns3_python_bindings()
