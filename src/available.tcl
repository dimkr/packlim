proc packlim.available {pkgs} {
	foreach name [dict keys $pkgs] {
		set pkg $pkgs($name)
		puts "$pkg(name)|$pkg(ver)|$pkg(desc)"
	}
}
