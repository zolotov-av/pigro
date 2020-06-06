<?php

const tab_width = 8;

$lines = file("avrdude.conf");

function expand_tabs($str)
{
    $result = array();
    $pos = 0;
    foreach(str_split($str) as $ch)
    {
        if ( $ch == "\t" )
        {
            for($i = ($pos % tab_width); $i < tab_width; $i++)
            {
                $pos++;
                $result[] = " ";
            }
        }
        else
        {
            $pos++;
            $result[] = $ch;
        }
    }
    return implode("", $result);
}

function getshift($str)
{
    $shift = 0;
    foreach(str_split($str) as $ch)
    {
        if ( $ch == ' ' ) $shift++;
        else break;
    }
    
    return $shift;
}

function is_comment($str)
{
    return substr($str, 0, 1) == "#";
}

function is_empty($str)
{
    return trim($str) == "" || is_comment($str);
}

function is_section($str)
{
    return ctype_alpha(substr($str, 0, 1));
}

function is_subsection($str, &$subsection, &$subsection_value)
{
    if ( preg_match("/^    ([a-zA-Z0-9]+)\s+\"([^\"]*)\"\s*$/s", $str, $match) )
    {
        $subsection = $match[1];
        $subsection_value = $match[2];
        return true;
    }
    return false;
}

function is_option($str)
{
    return getshift($str) == 4;
}

function is_param($str, &$param, &$value)
{
    /*
    if ( preg_match("/^    ([a-zA-Z_0-9]+)\s*=\s*\"([^\"]+)\"\s*;\s*$/s", $str, $match) )
    {
        $param = $match[1];
        $value = trim($match[2]);
        return true;
    }
    if ( preg_match("/^    ([a-zA-Z_0-9]+)\s*=\s*([^;]+);\s*$/s", $str, $match) )
    {
        $param = $match[1];
        $value = trim($match[2]);
        return true;
    }
    */
    if ( getshift($str) == 4 )
    {
        return parse_param($str, $param, $value);
    }
    return false;
}

function parse_param($str, &$param, &$value)
{
    if ( preg_match("/^\s*([a-zA-Z_0-9]+)\s*=\s*\"([^\"]+)\"\s*;\s*$/s", $str, $match) )
    {
        $param = $match[1];
        $value = trim($match[2]);
        return true;
    }
    if ( preg_match("/^\s*([a-zA-Z_0-9]+)\s*=\s*([^;]+);\s*$/s", $str, $match) )
    {
        $param = $match[1];
        $value = trim($match[2]);
        return true;
    }
    return false;
}

function is_suboption($str)
{
    return getshift($str) > 4;
}

function is_section_end($str)
{
    return trim($str) == ";";
}

function is_power_of_two(int $value)
{
    if ( $value <= 0 ) return false;
    while ( $value > 1 )
    {
        if ( $value % 2 == 1 ) return false;
        $value = intdiv($value, 2);
    }
    return true;
}

function parse_inthex($str)
{
    if ( preg_match("/^-?[0-9]+$/s", $str) )
    {
        return intval($str);
    }
    if ( preg_match("/^0x([0-9A-Fa-f]+)$/s", $str, $match) )
    {
        return intval($match[1], 16);
    }
    throw new Exception("parse_int($str) wrong value");
}

function parse_device_code($str)
{
    $result = array ();
    foreach(preg_split("/\s+/s", trim($str)) as $byte)
    {
        $value = parse_inthex($byte);
        if ( $value < 0 || $value > 255 )
        {
            throw new Exception("parse_device_code($str) wrong value");
        }
        $result[] = $value;
    }
    if ( count($result) != 3 )
    {
        throw new Exception("parse_device_code($str) wrong value: " . count($result));
    }
    return sprintf("0x%02X%02X%02X", $result[0], $result[1], $result[2]);
}

function make_ini_section($name, $config)
{
    $lines = array ();
    $lines[] = "\n";
    $lines[] = "[$name]\n";
    $lines[] = "\n";
    foreach($config as $key => $value)
    {
        $lines[] = "$key = $value\n";
    }
    return implode("", $lines);
}

function error($message)
{
    throw new Exception("$message");
}

try
{

$data = array ();

$lineno = 0;
$section = "";
$memory = "";

$part_id = 0;

foreach($lines as $line_x)
{
    $lineno ++;
    $line_t = expand_tabs($line_x);
    if ( is_empty($line_t) ) continue;
    if ( is_section($line_t) )
    {
        $section = trim($line_t);
        if ( $section == "part" )
        {
            $part_id ++;
            $data[$part_id]["line"] = $lineno;
        }
        
        //echo "section: $section\n";
        continue;
    }
    if ( $section == "part" )
    {
        $shift = getshift($line_t);
        $line = trim($line_t);
        if ( is_param($line_t, $param, $value) )
        {
            //echo "$lineno: param[$param] = $value\n";
            $data[$part_id]["param"][$param] = $value;
            continue;
        }
        if ( is_subsection($line_t, $subsection, $subsection_value) )
        {
            //echo "subsection: $subsection '$subsection_value'\n";
            if ( $subsection == "memory" )
            {
                $memory = $subsection_value;
            }
            else
            {
                $memory = "";
            }
            continue;
        }
        if ( is_option($line_t) )
        {
            //echo "$lineno: [option] $line\n";
            continue;
        }
        if ( is_suboption($line_t) )
        {
            //echo "$lineno: [suboption] $line\n";
            if ( $memory == "" ) continue;
            if ( parse_param($line_t, $param, $value) )
            {
                $data[$part_id]["memory"][$memory][$param] = $value;
            }
            continue;
        }
        if ( is_section_end($line_t) )
        {
            //echo "$lineno: [section_end] $section\n";
            $section = "";
            continue;
        }
        
        //echo "$lineno: [$shift] $line\n";
        
    }
}

$ini = array ();
$parts = array();
foreach($data as $part)
{
    if ( !isset($part["param"]["signature"]) ) continue;
    $signature = $part["param"]["signature"];
    
    if ( !isset($part["memory"]) )
    {
        print_r($part);
        error("error: memory not defined");
    }
    
    if ( !isset($part["memory"]["flash"]) )
    {
        print_r($part);
        error("error: flash memory not defined");
    }
    
    $name = strtolower($part["param"]["desc"]);
    
    $page_size = isset($part["memory"]["flash"]["page_size"]) ? parse_inthex($part["memory"]["flash"]["page_size"]) : -1;
    $page_count = isset($part["memory"]["flash"]["blocksize"]) ? parse_inthex($part["memory"]["flash"]["blocksize"]) : -1;
    
    if ( $page_size > 0 )
    {
        if ( ! is_power_of_two($page_size) )
        {
            print_r($part);
            throw new Exception("page_size is not power of 2");
        }
    }
    
    if ( $page_size > 0 )
    {
        if ( $page_size % 2 == 1 )
        {
            error("page_size[$signature] is odd: $page_size");
        }
        else
        {
            $page_size = intdiv($page_size, 2);
        }
    }
    
    $config = array (
        "name" => $part["param"]["desc"],
        "device_code" => parse_device_code($part["param"]["signature"]),
        "page_size" => $page_size,
        "page_count" => $page_count
    );
    
    if ( isset($parts[$name]) )
    {
        error("duplicate part '$name'");
    }
    
    $parts[$name] = $config;
    $ini[] = make_ini_section($name, $config);
}

//print_r($parts);
file_put_contents("avrdude.ini", $ini);

echo "\ndone\n\n";

}
catch (Exception $e)
{

    $message = $e->getMessage();
    echo "\nerror: $message\n\n";

}
