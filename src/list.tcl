proc packlim.parse {entry} {
	set fields [split $entry |]

	set pkg [dict create]
	dict set pkg name [lindex $fields 0]
	dict set pkg ver [lindex $fields 1]
	dict set pkg desc [lindex $fields 2]
	dict set pkg file [lindex $fields 3]
	dict set pkg deps [lindex $fields 4]

	return $pkg
}

proc packlim.entry {path} {
	set fp [open $path r]
	set entry [$fp read]
	$fp close

	packlim.parse $entry
}

proc packlim.list {path} {
	set fp [open $path r]
	set entries [$fp read]
	$fp close

	set pkgs [dict create]

	foreach entry [split $entries \n] {
		if {0 == [string length $entry]} {
			continue
		}
		set pkg [packlim.parse $entry]
		dict set pkgs $pkg(name) $pkg
	}

	return $pkgs
}
