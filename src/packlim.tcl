proc packlim_usage {err} {
	puts stderr "packlim_usage: [lindex $::argv 0] $err"
	exit 1
}

proc packlim_main {} {
	if {2 > $::argc} {
		packlim_usage "update|available|installed|install|remove|lock|source \[ARG\]..."
	}

	set env [env]
	set ::log [packlim.log packlim]
	try {
		$::log level $env(LOG)
	} on error {msg args} {
	}

	try {
		set pfix $env(PREFIX)
		append pfix /
	} on error {msg args} {
		set pfix /
	}
	file mkdir "${pfix}var/packlim/downloaded" "${pfix}var/packlim/installed" "${pfix}etc/packlim"

	if {[file exists "${pfix}etc/packlim/pub_key"]} {
		set fp [open "${pfix}etc/packlim/pub_key" r]
		set key [$fp read -nonewline]
		$fp close
	} else {
		set key ""
	}

	switch -exact [lindex $::argv 1] update {
		if {2 != $::argc} {
			packlim_usage update
		}

		set repo [packlim.repository $env(REPO)]
		set lock [packlim.wait "${pfix}var/packlim/lock"]
		packlim.update $repo $pfix
	} available {
		if {2 != $::argc} {
			packlim_usage available
		}

		set repo [packlim.repository $env(REPO)]
		set lock [packlim.wait "${pfix}var/packlim/lock"]
		set pkgs [packlim.cond_update $repo $pfix]

		packlim.available $pkgs
	} installed {
		if {2 != $::argc} {
			packlim_usage installed
		}

		packlim.installed $pfix
	} install {
		if {3 != $::argc} {
			packlim_usage "install NAME"
		}

		set repo [packlim.repository $env(REPO)]
		set lock [packlim.wait "${pfix}var/packlim/lock"]
		set pkgs [packlim.cond_update $repo $pfix]

		packlim.install [lindex $::argv 2] $pkgs $repo user $key $pfix
	} remove {
		if {3 != $::argc} {
			packlim_usage "remove NAME"
		}

		set lock [packlim.wait "${pfix}var/packlim/lock"]

		set name [lindex $::argv 2]
		if {[packlim.locked $name $pfix]} {
			$::log error "cannot remove $name; it is locked"
		} else {
			if {[packlim.needed $name $pfix]} {
				$::log error "cannot remove $name; it is required by another package"
			} else {
				packlim.remove $name $pfix
				packlim.cleanup $pfix
			}
		}
	} lock {
		if {3 != $::argc} {
			packlim_usage "lock NAME"
		}

		set lock [packlim.wait "${pfix}var/packlim/lock"]
		packlim.lock [lindex $::argv 2] $pfix
	} source {
		if {3 != $::argc} {
			packlim_usage "source SCRIPT"
		}

		source [lindex $::argv 2]
	} default {
		packlim_usage "update|available|installed|install|remove|lock|source \[ARG\]..."
	}

	$::log close
}
