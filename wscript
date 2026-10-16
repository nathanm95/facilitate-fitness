import os.path

top = '.'
out = 'build'


def options(ctx):
    ctx.load('pebble_sdk')


def configure(ctx):
    ctx.load('pebble_sdk')


def build(ctx):
    ctx.load('pebble_sdk')

    # Verification affordances; never ship a bundle built with any of these.
    #   FF_SHOW_TIME=1008  always show 10:08, for store shots: the emulator
    #                      follows the host clock and can't be set.
    #   FF_FAKE=1          show a fixed, believable day of health data instead
    #                      of the emulator's empty one.
    #   FF_THEME=<n>       force a theme (Theme enum order, see src/c/theme.h).
    #   FF_PCT=<n>         with FF_FAKE, set the step goal progress in percent.
    #   FF_USUAL=<steps>   with FF_FAKE, where the "usual by now" tick sits.
    #   FF_BPM=<n>         with FF_FAKE, the heart rate shown (emery).
    #   FF_CALIB=1         draw each font's "0" for tools/calib.py instead.
    #   FF_HR_PERIOD=<s>   switch the heart-rate sensor on (emery) so the
    #                      emulator accepts `pebble emu-heart-rate`.
    # They're appended to DEFINES here, in build(), so waf tracks them and a
    # change recompiles without `pebble clean`.
    flags = {}
    for name in ('FF_SHOW_TIME', 'FF_FAKE', 'FF_THEME', 'FF_PCT', 'FF_USUAL',
                 'FF_BPM', 'FF_CALIB', 'FF_HR_PERIOD'):
        if os.environ.get(name):
            flags[name] = int(os.environ[name])

    binaries = []
    cached_env = ctx.env
    for platform in ctx.env.TARGET_PLATFORMS:
        ctx.env = ctx.all_envs[platform]
        for name, value in flags.items():
            ctx.env.append_value('DEFINES', '%s=%d' % (name, value))
        ctx.set_group(ctx.env.PLATFORM_NAME)
        app_elf = '{}/pebble-app.elf'.format(ctx.env.BUILD_DIR)
        ctx.pbl_build(source=ctx.path.ant_glob('src/c/**/*.c'),
                      target=app_elf, bin_type='app')
        binaries.append({'platform': platform, 'app_elf': app_elf})
    ctx.env = cached_env

    ctx.set_group('bundle')
    ctx.pbl_bundle(binaries=binaries,
                   js=ctx.path.ant_glob(['src/pkjs/**/*.js',
                                         'src/pkjs/**/*.json']),
                   js_entry_file='src/pkjs/index.js')
