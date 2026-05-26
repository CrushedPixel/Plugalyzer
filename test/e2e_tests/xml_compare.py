from pathlib import Path
import xml.etree.ElementTree as ET
from typing import Tuple

def compare_xml_with_tolerance(actual_xml: str, expected_xml: str, tolerance: float = 0.000002) -> Tuple[bool, str]:
    actual_root = ET.fromstring(actual_xml)
    expected_root = ET.fromstring(expected_xml)
    
    result, msg = _compare_elements(actual_root, expected_root, tolerance)
    if not result:
        return False, msg

    return True, ""

def _compare_elements(actual: ET.Element, expected: ET.Element, tolerance: float) -> Tuple[bool, str]:
    if actual.tag != expected.tag:
        return False, f"XML mismatched tag: {actual.tag}/{expected.tag}"
    
    if not _compare_attributes(actual.attrib, expected.attrib, tolerance):
        return False, f"XML mismatched attribute: {actual.attrib}/{expected.attrib}"
    
    if (actual.text or "").strip() != (expected.text or "").strip():
        return False, f"XML mismatched content: {actual.text}/{expected.text}"
    
    actual_children = list(actual)
    expected_children = list(expected)
    
    if len(actual_children) != len(expected_children):
        return False, f"XML mismatched nodes: {len(actual_children)}/{len(expected_children)}"
    
    for actual_child, expected_child in zip(actual_children, expected_children):
        result, msg = _compare_elements(actual_child, expected_child, tolerance)
        if not result:
            return False, msg
    
    return True, ""

def _compare_attributes(actual: dict, expected: dict, tolerance: float) -> Tuple[bool, str]:
    if set(actual.keys()) != set(expected.keys()):
        return False, f"XML mismatched attributes: {actual}/{expected}"
    
    for key in expected.keys():
        result, msg = _compare_values(actual[key], expected[key], tolerance)
        if not result:
            return False, msg
    
    return True, ""

def _compare_values(actual: str, expected: str, tolerance: float) -> Tuple[bool, str]:
    try:
        actual_float = float(actual)
        expected_float = float(expected)
        within_tolerance = abs(actual_float - expected_float) <= tolerance
        if not within_tolerance:
            return False, f"XML mismatched values: {actual_float:.9f}/{expected_float:.9f}"
        return True, ""
    except ValueError:
        result = actual == expected
        if not result:
            return False, f"XML mismatched values: {actual}/{expected}"
        return True, ""
