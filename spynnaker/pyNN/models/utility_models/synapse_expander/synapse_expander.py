import logging
import os
from spinn_utilities.progress_bar import ProgressBar
from spinnman.model import ExecutableTargets
from spinnman.model.enums import CPUState
from spynnaker.pyNN.models.neuron import AbstractPopulationVertex
from spynnaker.pyNN.models.utility_models import DelayExtensionVertex
from spynnaker.pyNN.exceptions import SpynnakerException
from spynnaker.pyNN.models.neural_projections import ProjectionApplicationEdge
from spinn_utilities.make_tools.replacer import Replacer

logger = logging.getLogger(__name__)

SYNAPSE_EXPANDER = "synapse_expander.aplx"
DELAY_EXPANDER = "delay_expander.aplx"


def synapse_expander(
        app_graph, graph_mapper, placements, transceiver,
        provenance_file_path, executable_finder):
    """ Run the synapse expander - needs to be done after data has been loaded
    """

    synapse_expander = executable_finder.get_executable_path(SYNAPSE_EXPANDER)
    delay_expander = executable_finder.get_executable_path(DELAY_EXPANDER)

    progress = ProgressBar(len(app_graph.vertices) + 2, "Expanding Synapses")

    # Find the places where the synapse expander and delay receivers should run
    expander_cores = ExecutableTargets()
    for vertex in progress.over(app_graph.vertices, finish_at_end=False):

        # Find population vertices
        if isinstance(
                vertex, (AbstractPopulationVertex, DelayExtensionVertex)):

            # Add all machine vertices of the population vertex to ones
            # that need synapse expansion
            for m_vertex in graph_mapper.get_machine_vertices(vertex):
                vertex_slice = graph_mapper.get_slice(m_vertex)
                if vertex.gen_on_machine(vertex_slice):
                    placement = placements.get_placement_of_vertex(m_vertex)
                    if isinstance(vertex, AbstractPopulationVertex):
                        binary = synapse_expander
                    else:
                        binary = delay_expander
                    expander_cores.add_processor(
                        binary, placement.x, placement.y, placement.p)

    # Launch the delay receivers
    expander_app_id = transceiver.app_id_tracker.get_new_id()
    transceiver.execute_application(expander_cores, expander_app_id)
    progress.update()

    # Wait for everything to finish
    finished = False
    try:
        transceiver.wait_for_cores_to_be_in_state(
            expander_cores.all_core_subsets, expander_app_id,
            [CPUState.FINISHED])
        progress.update()
        finished = True
        _fill_in_connection_data(app_graph, graph_mapper)
        _extract_iobuf(expander_cores, transceiver, provenance_file_path)
        progress.end()
    except Exception:
        logger.exception("Synapse expander has failed")
        _handle_failure(
            expander_cores, transceiver, provenance_file_path)
    finally:
        transceiver.stop_application(expander_app_id)
        transceiver.app_id_tracker.free_id(expander_app_id)

        if not finished:
            raise SpynnakerException(
                "The synapse expander failed to complete")


def _extract_iobuf(expander_cores, transceiver, provenance_file_path,
                   display=False):
    """ Extract IOBuf from the cores
    """
    io_buffers = transceiver.get_iobuf(expander_cores.all_core_subsets)
    core_to_replacer = dict()
    for binary in expander_cores.binaries:
        replacer = Replacer(binary)
        for core_subset in expander_cores.get_cores_for_binary(binary):
            x = core_subset.x
            y = core_subset.y
            for p in core_subset.processor_ids:
                core_to_replacer[x, y, p] = replacer

    for io_buf in io_buffers:
        file_path = os.path.join(
            provenance_file_path, "expander_{}_{}_{}.txt".format(
                io_buf.x, io_buf.y, io_buf.p))
        replacer = core_to_replacer[io_buf.x, io_buf.y, io_buf.p]
        text = ""
        for line in io_buf.iobuf.split("\n"):
            text += replacer.replace(line) + "\n"
        with open(file_path, "w") as writer:
            writer.write(text)
        if display:
            print("{}:{}:{} {}".format(io_buf.x, io_buf.y, io_buf.p, text))


def _handle_failure(expander_cores, transceiver, provenance_file_path):
    """ Handle failure of the expander

    :param executable_targets:
    :param txrx:
    :param provenance_file_path:
    :rtype: None
    """
    core_subsets = expander_cores.all_core_subsets
    error_cores = transceiver.get_cores_not_in_state(
        core_subsets, [CPUState.RUNNING, CPUState.FINISHED])
    logger.error(transceiver.get_core_status_string(error_cores))
    _extract_iobuf(expander_cores, transceiver, provenance_file_path,
                   display=True)

def _fill_in_connection_data(app_graph, graph_mapper):
    """ Once connection has run, fill in the connection data
    :param app_graph
    :param graph_mapper
    :rtype: None
    """
    for app_edge in app_graph.edges:
        if isinstance(app_edge, ProjectionApplicationEdge):
            synapse_info = app_edge.synapse_information
            print('app_edge, synapse_info: ', app_edge, synapse_info)

    for app_vertex in app_graph.vertices:
        if isinstance(app_edge, AbstractPopulationVertex):
            print('app_vertex: ', app_vertex)
#         if (app_edge, synapse_info) in self.__pre_run_connection_holders:
#             for conn_holder in self.__pre_run_connection_holders[
#                     app_edge, synapse_info]:
#                 conn_holder.add_connections(self._read_synapses(
#                     synapse_info, pre_vertex_slice, post_vertex_slice,
#                     row_length, delayed_row_length, n_synapse_types,
#                     weight_scales, row_data, delayed_row_data,
#                     app_edge.n_delay_stages, machine_time_step))
#                 conn_holder.finish()
