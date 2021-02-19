# Set moo schema search path
from dunedaq.env import get_moo_model_path
import moo.io

# PAR 2021-02-17
#
# This is a hideous hack to work around
# https://github.com/DUNE-DAQ/daq-cmake/issues/32 . Our test schema
# are not installed into the build directory, and so they're not in
# moo's path. So we find the *source* test/schema directory relative
# to the current file, and put that in moo's path. It would have been
# marginally less hideous to use $DBT_AREA_ROOT, but that variable is
# not exported, so we can't see it.
#
# When the daq-cmake bug above is fixed, this can go away
import pathlib
moo.io.default_load_path = [pathlib.Path(__file__).absolute().parents[2] / "test" / "schema"] + get_moo_model_path()

# Load configuration types
import moo.otypes
moo.otypes.load_types('appfwk-cmd-schema.jsonnet')
moo.otypes.load_types('trigemu-TriggerDecisionEmulator-schema.jsonnet')
moo.otypes.load_types('trigemu-FakeTimeSyncSource-schema.jsonnet')
moo.otypes.load_types('trigemu-FakeInhibitGenerator-schema.jsonnet')
moo.otypes.load_types('trigemu-FakeTokenGenerator-schema.jsonnet')

# Import new types
import dunedaq.appfwk.cmd as cmd # AddressedCmd,
import dunedaq.trigemu.triggerdecisionemulator as tde
import dunedaq.trigemu.faketimesyncsource as ftss
import dunedaq.trigemu.fakeinhibitgenerator as fig
import dunedaq.trigemu.faketokengenerator as ftg

from appfwk.utils import mcmd, mspec

import json
import math
# Time to waait on pop()
QUEUE_POP_WAIT_MS = 100
# local clock speed Hz
CLOCK_SPEED_HZ = 50000000

def generate(NUMBER_OF_DATA_PRODUCERS=2,          
        DATA_RATE_SLOWDOWN_FACTOR=10,
        RUN_NUMBER=333, 
        TRIGGER_RATE_HZ=1.0,
        INHIBITS_ENABLED=False,
        TOKENS_ENABLED=True):
    
    trigger_interval_ticks = math.floor((1 / TRIGGER_RATE_HZ) * CLOCK_SPEED_HZ / DATA_RATE_SLOWDOWN_FACTOR)

    # Define modules and queues
    queue_bare_specs = [cmd.QueueSpec(inst="time_sync_q", kind='FollyMPMCQueue', capacity=100),
            cmd.QueueSpec(inst="token_q", kind="FollySPSCQueue", capacity=20)]

    if INHIBITS_ENABLED:
        queue_bare_specs += [cmd.QueueSpec(inst="trigger_inhibit_q", kind='FollySPSCQueue', capacity=20)]
    if TOKENS_ENABLED:
        queue_bare_specs += [cmd.QueueSpec(inst="trigger_decision_q", kind='FollySPSCQueue', capacity=20)]
    

    # Only needed to reproduce the same order as when using jsonnet
    queue_specs = cmd.QueueSpecs(sorted(queue_bare_specs, key=lambda x: x.inst))


    mod_specs = [mspec("ftss", "FakeTimeSyncSource", [cmd.QueueInfo(name="time_sync_sink", inst="time_sync_q", dir="output")]),
        mspec("frr", "FakeRequestReceiver", [cmd.QueueInfo(name="trigger_decision_source", inst="trigger_decision_q", dir="input")])] 

    if INHIBITS_ENABLED:
        mod_specs += [mspec("fig", "FakeInhibitGenerator", [cmd.QueueInfo(name="trigger_inhibit_sink", inst="trigger_inhibit_q", dir="output")])]
        if not TOKENS_ENABLED:
            mod_specs += [mspec("tde", "TriggerDecisionEmulator", [cmd.QueueInfo(name="time_sync_source", inst="time_sync_q", dir="input"),
                        cmd.QueueInfo(name="trigger_inhibit_source", inst="trigger_inhibit_q", dir="input"),
                        cmd.QueueInfo(name="trigger_decision_sink", inst="trigger_decision_q", dir="output")])]
    if TOKENS_ENABLED:
        mod_specs += [mspec("ftg", "FakeTokenGenerator", [cmd.QueueInfo(name="token_sink", inst="token_q", dir="output")]),]
        if not INHIBITS_ENABLED:
            mod_specs += [mspec("tde", "TriggerDecisionEmulator", [cmd.QueueInfo(name="time_sync_source", inst="time_sync_q", dir="input"),
                        cmd.QueueInfo(name="token_source", inst="token_q", dir="input"),
                        cmd.QueueInfo(name="trigger_decision_sink", inst="trigger_decision_q", dir="output")])]
    if TOKENS_ENABLED and INHIBITS_ENABLED:
            mod_specs += [mspec("tde", "TriggerDecisionEmulator", [cmd.QueueInfo(name="time_sync_source", inst="time_sync_q", dir="input"),
                        cmd.QueueInfo(name="trigger_inhibit_source", inst="trigger_inhibit_q", dir="input"),
                        cmd.QueueInfo(name="token_source", inst="token_q", dir="input"),
                        cmd.QueueInfo(name="trigger_decision_sink", inst="trigger_decision_q", dir="output")])]


    init_specs = cmd.Init(queues=queue_specs, modules=mod_specs)

    jstr = json.dumps(init_specs.pod(), indent=4, sort_keys=True)
    print(jstr)

    initcmd = cmd.Command(id=cmd.CmdId("init"),
        data=init_specs)


    confcmd = mcmd("conf", [("tde", tde.ConfParams(links=[idx for idx in range(NUMBER_OF_DATA_PRODUCERS)],
                        min_links_in_request=NUMBER_OF_DATA_PRODUCERS,
                        max_links_in_request=NUMBER_OF_DATA_PRODUCERS,
                        min_readout_window_ticks=1200,
                        max_readout_window_ticks=1200,
                        trigger_window_offset=1000,
                        # The delay is set to put the trigger well within the
                        # latency buff
                        trigger_delay_ticks=math.floor(2 * CLOCK_SPEED_HZ / DATA_RATE_SLOWDOWN_FACTOR),
                        # We divide the trigger interval by
                        # DATA_RATE_SLOWDOWN_FACTOR so the triggers are still
                        # emitted per (wall-clock) second, rather than being
                        # spaced out further
                        trigger_interval_ticks=trigger_interval_ticks,
                        clock_frequency_hz=CLOCK_SPEED_HZ / DATA_RATE_SLOWDOWN_FACTOR)),
                ("ftss", ftss.ConfParams(sync_interval_ticks=64000000)),
                ("fig", fig.ConfParams(inhibit_interval_ms=5000)),
                ("ftg", ftg.ConfParams(token_interval_ms=math.floor(1000 / TRIGGER_RATE_HZ), token_sigma_ms=math.floor(1 / TRIGGER_RATE_HZ), initial_tokens=10))])
    
    jstr = json.dumps(confcmd.pod(), indent=4, sort_keys=True)
    print(jstr)

    startpars = cmd.StartParams(run=RUN_NUMBER)
    startcmd = mcmd("start", [("frr", startpars), ("tde", startpars), ("ftss",startpars), ("fig", startpars), ("ftg", startpars),])

    jstr = json.dumps(startcmd.pod(), indent=4, sort_keys=True)
    print("=" * 80 + "\nStart\n\n", jstr)

    emptypars = cmd.EmptyParams()

    stopcmd = mcmd("stop", [("tde", emptypars),("frr", emptypars), ("ftss", emptypars), ("fig", emptypars), ("ftg",emptypars)])

    jstr = json.dumps(stopcmd.pod(), indent=4, sort_keys=True)
    print("=" * 80 + "\nStop\n\n", jstr)

    pausecmd = mcmd("pause", [("", emptypars)])

    jstr = json.dumps(pausecmd.pod(), indent=4, sort_keys=True)
    print("=" * 80 + "\nPause\n\n", jstr)

    resumecmd = mcmd("resume", [("tde", tde.ResumeParams(trigger_interval_ticks=trigger_interval_ticks))])

    jstr = json.dumps(resumecmd.pod(), indent=4, sort_keys=True)
    print("=" * 80 + "\nResume\n\n", jstr)

    scrapcmd = mcmd("scrap", [("", emptypars)])

    jstr = json.dumps(scrapcmd.pod(), indent=4, sort_keys=True)
    print("=" * 80 + "\nScrap\n\n", jstr)

    # Create a list of commands
    cmd_seq = [initcmd, confcmd, startcmd, stopcmd, pausecmd, resumecmd, scrapcmd]

    # Print them as json (to be improved/moved out)
    jstr = json.dumps([c.pod() for c in cmd_seq], indent=4, sort_keys=True)
    return jstr
        
if __name__ == '__main__':
    # Add -h as default help option
    CONTEXT_SETTINGS = dict(help_option_names=['-h', '--help'])

    import click

    @click.command(context_settings=CONTEXT_SETTINGS)
    @click.option('-n', '--number-of-data-producers', default=2)
    @click.option('-s', '--data-rate-slowdown-factor', default=10)
    @click.option('-r', '--run-number', default=333)
    @click.option('-t', '--trigger-rate-hz', default=1.0)
    @click.option('--inhibits-enabled', is_flag=True)
    @click.option('--tokens-disabled', is_flag=True)
    @click.argument('json_file', type=click.Path(), default='trigemu-fake-app.json')
    def cli(number_of_data_producers, data_rate_slowdown_factor, run_number, trigger_rate_hz, inhibits_enabled, tokens_disabled, json_file):
        """
          JSON_FILE: Input raw data file.
          JSON_FILE: Output json configuration file.
        """

        with open(json_file, 'w') as f:
            f.write(generate(NUMBER_OF_DATA_PRODUCERS = number_of_data_producers,
                    DATA_RATE_SLOWDOWN_FACTOR = data_rate_slowdown_factor,
                    RUN_NUMBER = run_number, 
                    TRIGGER_RATE_HZ = trigger_rate_hz,
                    INHIBITS_ENABLED = inhibits_enabled,
                    TOKENS_ENABLED = not tokens_disabled))

        print(f"'{json_file}' generation completed.")

    cli()
    
