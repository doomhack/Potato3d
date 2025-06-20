# Copyright (C) 2016 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

# This is a place to add your own dumpers for testing purposes.
# Any contents here will be picked up by GDB, LLDB, and CDB based
# debugging in Qt Creator automatically.

# NOTE: This file will get overwritten when updating Qt Creator.
#
# To add dumpers that don't get overwritten, copy this file here
# to a safe location outside the Qt Creator installation and
# make this location known to Qt Creator using the Debugger >
# Locals & Expressions > Extra Debugging Helpers setting.

# Example to display a simple type
# template<typename U, typename V> struct MapNode
# {
#     U key;
#     V data;
# }
#
# def qdump__MapNode(d, value):
#    d.putValue("This is the value column contents")
#    d.putExpandable()
#    if d.isExpanded():
#        with Children(d):
#            # Compact simple case.
#            d.putSubItem("key", value["key"])
#            # Same effect, with more customization possibilities.
#            with SubItem(d, "data")
#                d.putItem("data", value["data"])

# Check http://doc.qt.io/qtcreator/creator-debugging-helpers.html
# for more details or look at qttypes.py, stdtypes.py, boosttypes.py
# for more complex examples.

from dumper import Children, SubItem, UnnamedSubItem, DumperBase
from utils import DisplayFormat, TypeCode

######################## Your code below #######################

def qdump__P3D__FP(d, value):
    # Get the template parameter (fracbits)
    innerType = value.type
    fracbits = innerType.templateArgument(0)
    
    # Get the private member 'n'
    n = value["n"].integer()
    
    # Calculate the floating point value
    divisor = 1 << fracbits
    fp_value = float(n) / float(divisor)
    
    # Display format: show both the floating point value and raw integer
    d.putValue("%.6f (raw: %d)" % (fp_value, n))
    d.putPlainChildren(value)
    
    # Add child items for detailed view
    d.putNumChild(2)
    if d.isExpanded():
        with Children(d):
            d.putSubItem("floating_value", fp_value)
            d.putSubItem("raw_value", n)
            d.putSubItem("fracbits", fracbits)