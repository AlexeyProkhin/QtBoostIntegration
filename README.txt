QtBoostIntegration is a small library which enables you to connect anything that
can be stored in a boost::function to a Qt signal. This includes function pointers,
Boost.Lambda expressions, and Boost.Bind expressions, as well as C++-0x lambda functions.

If QtBoostIntegration proves popular, I'll try to submit it for inclusion in Qt itself.
In the meantime, I would suggest that you simply embed it in any project that needs it.
QtBoostIntegration has no build dependencies other than Qt 4.x itself, but it requires
Boost.Preprocessor and Boost.Function to actually use it, including building the tests.

QtBoostIntegration is MIT licensed so that you can embed it without license issues,
though of course if you have bugfixes or more tests, I hope you'll contribute them!
