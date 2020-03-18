
R"===<><>===(
namespace eval psn {
    namespace eval sta {
        sta::define_sta_cmds
        namespace import ::sta::*
        namespace export *
        namespace ensemble create
    }
    namespace export *
    namespace ensemble create
}

)===<><>==="