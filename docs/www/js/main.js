$(function () {
    // Open external links in new tab
    $('a').each(function (index, a) {
        a = $(a);
        if (a.attr('href').indexOf('//') != -1)
            a.attr('target', '_blank');
    });
})
