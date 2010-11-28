TEMPLATE = subdirs

SUBDIRS = src

!isEmpty(BOOST_DIR) {
    SUBDIRS += test
    message($$sprintf("Building tests using Boost in %1", $$BOOST_DIR))
} else {
    message("No BOOST_DIR specified, not building tests");
}

