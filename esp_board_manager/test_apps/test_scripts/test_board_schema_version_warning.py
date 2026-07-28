import logging
import sys


def test_codec_dev_upgrade_does_not_add_a_board_schema_version(bmgr_root, caplog):
    sys.path.insert(0, str(bmgr_root))
    from generators.utils.board_schema_version import warn_if_invalid_board_yaml_schema_version

    logger = logging.getLogger('test_audio_codec_v2_schema_version')
    with caplog.at_level(logging.WARNING, logger=logger.name):
        warn_if_invalid_board_yaml_schema_version(logger, '2.0.0', 'board_devices.yaml device #1')

    assert 'Unrecognized board YAML schema `version`' in caplog.text
    assert '1.0.0' in caplog.text


def test_invalid_board_yaml_schema_version_warning_uses_standard_prefix(bmgr_root, caplog):
    sys.path.insert(0, str(bmgr_root))
    from generators.utils.board_schema_version import warn_if_invalid_board_yaml_schema_version
    logger = logging.getLogger('test_board_schema_version_warning')

    with caplog.at_level(logging.WARNING, logger=logger.name):
        warn_if_invalid_board_yaml_schema_version(logger, '2.0', 'board_peripherals.yaml peripheral #1')

    assert '⚠️  WARNING: Unrecognized board YAML schema `version`' in caplog.text
    assert 'board_peripherals.yaml peripheral #1' in caplog.text
    assert 'schema generations 1.0.0' in caplog.text
