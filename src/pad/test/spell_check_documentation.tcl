exec aspell --lang=en create master ./en-custom.rws < ICeWall.wordlist
foreach file [glob -nocomplain ../doc/*.md] {
  exec cat $file | aspell --ignore-case list --extra-dicts=./en-custom.rws | sort -u 
}
