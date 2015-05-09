proc packlim.installed {pfix} {
	foreach path [glob -nocomplain -directory "${pfix}var/packlim/installed" *] {
		set pkg [packlim.entry "$path/entry"]
		puts "$pkg(name)|$pkg(ver)|$pkg(desc)"
	}
}
