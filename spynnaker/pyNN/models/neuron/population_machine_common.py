# Copyright (c) 2017-2020The University of Manchester
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
from collections import namedtuple
from spinn_utilities.overrides import overrides
from pacman.model.graphs.machine import MachineVertex

from spinn_front_end_common.interface.provenance import (
    ProvidesProvenanceDataFromMachineImpl)
from spinn_front_end_common.interface.buffer_management.buffer_models import (
    AbstractReceiveBuffersToHost)
from spinn_front_end_common.utilities.helpful_functions import (
    locate_memory_region_for_placement)
from spinn_front_end_common.interface.profiling.profile_utils import (
    get_profiling_data, reserve_profile_region, write_profile_region_data)
from spinn_front_end_common.abstract_models import AbstractHasAssociatedBinary
from spinn_front_end_common.interface.buffer_management\
    .recording_utilities import (
        get_recording_header_size, get_recording_header_array)
from spinn_front_end_common.interface.simulation.simulation_utilities import (
    get_simulation_header_array)

from spinn_front_end_common.utilities.utility_objs import ExecutableType
from spinn_front_end_common.interface.profiling import AbstractHasProfileData
from spinn_front_end_common.utilities.constants import SIMULATION_N_BYTES


# Identifiers for common regions
CommonRegions = namedtuple(
    "CommonRegions",
    ["system", "provenance", "profile", "recording"])


class PopulationMachineCommon(
        MachineVertex,
        ProvidesProvenanceDataFromMachineImpl,
        AbstractReceiveBuffersToHost,
        AbstractHasProfileData,
        AbstractHasAssociatedBinary):
    """ A common machine vertex for all population binaries
    """

    __slots__ = [
        # Resources used by the machine vertex
        "__resources",
        # Regions to be used
        "__regions",
        # The total number of provenance items returned by this core
        "__n_provenance_items",
        # The profile tags to be decoded
        "__profile_tags",
        # The name of the binary to run on the core
        "__binary_file_name"
    ]

    def __init__(
            self, label, constraints, app_vertex, vertex_slice, resources,
            regions, n_provenance_items, profile_tags, binary_file_name):
        """
        :param str label: The label of the vertex
        :param list(~pacman.model.constraints.AbstractConstraint) constraints:
            Constraints for the vertex
        :param AbstractPopulationVertex app_vertex:
            The associated application vertex
        :param ~pacman.model.graphs.common.Slice vertex_slice:
            The slice of the population that this implements
        :param ~pacman.model.resources.ResourceContainer resources:
            The resources used by the vertex
        :param .CommonRegions regions: The regions to be assigned
        :param int n_provenance_items:
            The number of additional provenance items to be read
        :param dict(int-->str) profile_tags:
            A mapping of profile identifiers to names
        :param str binary_file_name: The name of the binary file
        """
        super(PopulationMachineCommon, self).__init__(
            label, constraints, app_vertex, vertex_slice)
        self.__resources = resources
        self.__regions = regions
        self.__n_provenance_items = n_provenance_items
        self.__profile_tags = profile_tags
        self.__binary_file_name = binary_file_name

    @property
    @overrides(MachineVertex.resources_required)
    def resources_required(self):
        return self.__resources

    @property
    @overrides(ProvidesProvenanceDataFromMachineImpl._provenance_region_id)
    def _provenance_region_id(self):
        return self.__regions.provenance

    @property
    @overrides(ProvidesProvenanceDataFromMachineImpl._n_additional_data_items)
    def _n_additional_data_items(self):
        return self.__n_provenance_items

    @overrides(AbstractReceiveBuffersToHost.get_recording_region_base_address)
    def get_recording_region_base_address(self, txrx, placement):
        return locate_memory_region_for_placement(
            placement, self.__regions.recording, txrx)

    @overrides(AbstractHasProfileData.get_profile_data)
    def get_profile_data(self, transceiver, placement):
        return get_profiling_data(
            self.__regions.profile, self.__profile_tags, transceiver,
            placement)

    @overrides(AbstractHasAssociatedBinary.get_binary_start_type)
    def get_binary_start_type(self):
        return ExecutableType.USES_SIMULATION_INTERFACE

    def _write_common_data_spec(self, spec, rec_regions):
        """ Write the data specification for the common regions

        :param ~data_specification.DataSpecificationGenerator spec:
            The data specification to write to
        :param list(int) rec_regions:
            A list of sizes of each recording region (including empty ones)
        """
        # Write the setup region
        spec.reserve_memory_region(
            region=self.__regions.system, size=SIMULATION_N_BYTES,
            label='System')
        spec.switch_write_focus(self.__regions.system)
        spec.write_array(get_simulation_header_array(self.__binary_file_name))

        # Reserve memory for provenance
        self.reserve_provenance_data_region(spec)

        # Write profile data
        reserve_profile_region(
            spec, self.__regions.profile, self._app_vertex.n_profile_samples)
        write_profile_region_data(
            spec, self.__regions.profile, self._app_vertex.n_profile_samples)

        # Set up for recording
        spec.reserve_memory_region(
            region=self.__regions.recording,
            size=get_recording_header_size(len(rec_regions)),
            label="Recording")
        spec.switch_write_focus(self.__regions.recording)
        spec.write_array(get_recording_header_array(rec_regions))

    @overrides(AbstractHasAssociatedBinary.get_binary_file_name)
    def get_binary_file_name(self):
        return self.__binary_file_name
