###############################################################################
##
## BSD 3-Clause License
##
## Copyright (c) 2019, The Regents of the University of California
## All rights reserved.
##
## Redistribution and use in source and binary forms, with or without
## modification, are permitted provided that the following conditions are met:
##
## * Redistributions of source code must retain the above copyright notice, this
##   list of conditions and the following disclaimer.
##
## * Redistributions in binary form must reproduce the above copyright notice,
##   this list of conditions and the following disclaimer in the documentation
##   and#or other materials provided with the distribution.
##
## * Neither the name of the copyright holder nor the names of its
##   contributors may be used to endorse or promote products derived from
##   this software without specific prior written permission.
##
## THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
## AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
## IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
## ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
## LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
## CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
## SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
## INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
## CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
## ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
## POSSIBILITY OF SUCH DAMAGE.
##
###############################################################################

read_lef input.lef
read_def input.def

tapcell -add_boundary_cell
        -tap_nwin2_master "TAPCELL_NWIN2_MASTER_NAME"
        -tap_nwin3_master "TAPCELL_NWIN3_MASTER_NAME"
        -tap_nwout2_master "TAPCELL_NWOUT2_MASTER_NAME"
        -tap_nwout3_master "TAPCELL_NWOUT3_MASTER_NAME"
        -tap_nwintie_master "TAPCELL_NWINTIE_MASTER_NAME"
        -tap_nwouttie_master "TAPCELL_NWOUTTIE_MASTER_NAME"
        -cnrcap_nwin_master "CNRCAP_NWIN_MASTER_NAME"
        -cnrcap_nwout_master "CNRCAP_NWOUT_MASTER_NAME"
        -incnrcap_nwin_master "INCNRCAP_NWIN_MASTER_NAME"
        -incnrcap_nwout_master "INCNRCAP_NWOUT_MASTER_NAME"
        -endcap_master "ENDCAP_MASTER_NAME"
        -tapcell_master "TAPCELL_MASTER_NAME"
        -distance "25"
        -halo_width_x "2"
        -halo_width_y "2"

write_def tap.def
