# -*- Mode: python; py-indent-offset: 4; indent-tabs-mode: nil; coding: utf-8; -*-

def build(bld):
    module = bld.create_ns3_module('sally', ['olsr', 'aodv'])
    module.includes = '.'
    module.source = [
        'model/sally-routing-protocol.cc',
        'helper/sally-helper.cc',
        ]

    module_test = bld.create_ns3_module_test_library('sally')
    module_test.source = [
        'test/sally-test-suite.cc',
        ]

    headers = bld(features='ns3header')
    headers.module = 'sally'
    headers.source = [
        'model/sally-routing-protocol.h',
        'helper/sally-helper.h',
        ]

    if bld.env.ENABLE_EXAMPLES:
        bld.recurse('examples')

    # bld.ns3_python_bindings()

