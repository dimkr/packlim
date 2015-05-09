proc packlim.update {repo pfix} {
	$::log info "fetching the package list"
	$repo fetch available "${pfix}var/packlim/available"
}

proc packlim.cond_update {repo pfix} {
	set path "${pfix}var/packlim/available"
	if {![file exists $path]} {
		$::log info "fetching the package list"
		$repo fetch available $path
	}
	packlim.list $path
}
