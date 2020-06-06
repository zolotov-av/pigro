<?php

function is_empty($str)
{
    if ( trim($str) == "" ) return true;
    if ( preg_match("/^\s*#/s", $str) ) return true;
}

function is_section($str, &$section)
{
    if ( preg_match("/^\s*([0-9_A-Za-z]+)\s*$/s", $str, $match) )
    {
        $section = $match[1];
        return true;
    }
    return false;
}

function is_subsection($str, &$subsection, &$subsection_value)
{
    if ( preg_match("/^\s*([a-zA-Z0-9]+)\s+\"([^\"]*)\"\s*$/s", $str, $match) )
    {
        $subsection = $match[1];
        $subsection_value = $match[2];
        return true;
    }
    return false;
}

function is_section_end($str)
{
    return trim($str) == ";";
}

function is_param($str, &$param, &$value)
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

function parse_bool($str)
{
    $value = strtolower($str);
    if ( $str == "yes" ) return true;
    if ( $str == "no" ) return false;
    error("parse_bool($str) wrong value");
}

function parse_int($str)
{
    if ( preg_match("/^-?[0-9]+$/s", $str) )
    {
        return intval($str);
    }
    if ( preg_match("/^0x([0-9A-Fa-f]+)$/s", $str, $match) )
    {
        return intval($match[1], 16);
    }
    error("parse_int($str) wrong value");
}

function parse_device_code($str)
{
    $result = array ();
    foreach(preg_split("/\s+/s", trim($str)) as $byte)
    {
        $value = parse_int($byte);
        if ( $value < 0 || $value > 255 )
        {
            error("parse_device_code($str) wrong value");
        }
        $result[] = $value;
    }
    if ( count($result) != 3 )
    {
        error("parse_device_code($str) wrong value: " . count($result));
    }
    return sprintf("0x%02X%02X%02X", $result[0], $result[1], $result[2]);
}

function flash_size(int $page_size, int $page_count)
{
    return $page_size * 2 * $page_count;
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

    $lines = file("avrdude.conf");

    $lineno = 0;
    $section = "";
    $subsection = "";
    $memory = "";

    $part_id = 0;
    $data = array ();

    foreach($lines as $line_t)
    {
        $lineno ++;
        if ( is_empty($line_t) ) continue;
        if ( is_section($line_t, $s) )
        {
            if ( $section != "" )
            {
                error("unexpected new section '$s', line: $lineno");
            }
            $section = $s;
            if ( $section == "part" )
            {
                $part_id ++;
                $data[$part_id]["line"] = $lineno;
            }
            continue;
        }
        if ( is_subsection($line_t, $s, $value) )
        {
            if ( $subsection != "" )
            {
                error("unexpected subsection: '$subsection', line: $line");
            }
            $subsection = $s;
            if ( $subsection == "memory" )
            {
                $memory = $value;
                if ( $memory == "" )
                {
                    error("empty memory name, line: $lineno");
                }
            }
            continue;
        }
        if ( is_section_end($line_t) )
        {
            if ( $subsection != "" )
            {
                //echo "close subsection: $subsection\n";
                $subsection = "";
                continue;
            }
            if ( $section != "" )
            {
                //echo "close section: $section\n";
                $section = "";
                continue;
            }
            error("unexpected end of section, line: $lineno");
            continue;
        }
        if ( is_param($line_t, $param, $value) )
        {
            if ( $section == "part" )
            {
                if ( $subsection == "" )
                {
                    $data[$part_id]["param"][$param] = $value;
                    continue;
                }
                if ( $subsection == "memory" )
                {
                    $data[$part_id]["memory"][$memory][$param] = $value;
                    continue;
                }
            }
        }
        //echo "warn: $line_t";
    }

    $ini = array ();
    $parts = array();
    foreach($data as $part)
    {
        if ( !isset($part["param"]["desc"]) )
        {
            echo "skip: part (line: $part[line]) have not description\n";
            continue;
        }
        $name = strtolower($part["param"]["desc"]);

        if ( !isset($part["param"]["signature"]) )
        {
            echo "skip: part (line: $part[line]) have not signature\n";
            continue;
        }
        $signature = $part["param"]["signature"];
        $device_code = parse_device_code($part["param"]["signature"]);

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

        $paged = isset($part["memory"]["flash"]["paged"]) ? parse_bool($part["memory"]["flash"]["paged"]) : false;

        $config = array (
            "name" => $part["param"]["desc"],
            "device_code" => $device_code,
            "paged" => ($paged ? "yes" : "no")
        );

        if ( $paged )
        {
            $page_size = isset($part["memory"]["flash"]["page_size"]) ? parse_int($part["memory"]["flash"]["page_size"]) : 0;
            $page_count = isset($part["memory"]["flash"]["num_pages"]) ? parse_int($part["memory"]["flash"]["num_pages"]) : 0;

            if ( $page_size > 0 )
            {
                if ( $page_size % 2 == 1 )
                {
                    print_r($part);
                    error("page_size[$signature] is odd: $page_size");
                }
                else
                {
                    $page_size = intdiv($page_size, 2);
                }
                if ( ! is_power_of_two($page_size) )
                {
                    print_r($part);
                    error("page_size is not power of 2");
                }
            }

            $flash_size = flash_size($page_size, $page_count);
            if ( $flash_size > 0x10000 )
            {
                $sizek = intdiv($flash_size + 1023, 1024);
                echo "warn: $name have wrong or unsupported flash size: {$sizek}k/64k\n";
            }

            $config["page_size"] = $page_size;
            $config["page_count"] = $page_count;
        }

        if ( isset($parts[$name]) )
        {
            error("duplicate part '$name'");
        }

        $parts[$name] = $config;
        $ini[] = make_ini_section($name, $config);
    }

    file_put_contents("avrdude.ini", $ini);

    echo "\ndone\n\n";

}
catch (Exception $e)
{

    $message = $e->getMessage();
    echo "\nerror: $message\n\n";

}
