(function(){

     var hasClass = function(obj,className) {
         var re = new RegExp("(^|\\s)" + className + "(\\s|$)");
         return re.test(obj.className);
     };

     var get_blog_block = function() {
         var block;
         for(var i = 0;Math.min(i,19) != 19; i++) {
             block = document.getElementById('Blog'+i);
             if (!!block) break;
         }
         return block;
     };
     var change_links = function(post) {
         var spans = post.getElementsByTagName('span');
         var anchors, changelist = [], permalink = '', tempanchor, permalink_found = false, elements;
         for(var i = 0; Math.min(i, spans.length) != (spans.length); i++) {
             if ((hasClass(spans[i], 'post-timestamp') || hasClass(spans[i], 'disqus-blogger-permalink')) && !permalink_found) {
                 anchors = spans[i].getElementsByTagName('a');
                 for (var q=0; Math.min(q, anchors.length) != (anchors.length); q++) {
                     if (hasClass(anchors[q], 'timestamp-link') || hasClass(anchors[q], 'disqus-blogger-permalink-url')) {
                         permalink = anchors[q].href;
                         permalink_found = true;
                     }
                 }
             }
             if (hasClass(spans[i], 'post-comment-link') || hasClass(spans[i], 'disqus-blogger-comment-link'))
                 changelist.push(spans[i]);
         }
         //if not found, use slower method and iterate through every element in the post
         if (!permalink_found) {
             elements = post.getElementsByTagName('*');
             for(var k = 0; k < elements.length; k++) {
                 if ((hasClass(elements[k], 'entry-title')||hasClass(elements[k], 'post-title')) && !permalink_found) {
                     anchors = elements[k].getElementsByTagName('a');
                     for (var g=0; g < anchors.length; g++) {
                         if (!!anchors[g].href) {
                             permalink = anchors[g].href;
                             permalink_found = true;
                             break;
                         }
                     }
                 }
                 if (permalink_found) break;
             }
         }

         // if we still can't find a permalink, just skip this post and call it a loss
         if (!permalink_found) {
            return;
         }

         // with blogger's country specific tld changes, we need to check to see if the permalink we grabbed
         // matches a country specific tld.  to do so, we use the blog.homepageUrl and blog.homepageCanonicalUrl
         // we set in the actual widget html to check and do a search and replace
         // if the normal url and the canonical url match, then we know there isn't a country specific tld
         if ((disqus_blogger_homepage_url && disqus_blogger_canonical_homepage_url) &&
             (disqus_blogger_homepage_url != disqus_blogger_canonical_homepage_url)) {
             // make sure that the country tld is in the permalink
             if (permalink.match(disqus_blogger_homepage_url)) {
                // switch out country specific tld for canonical normal tld
                permalink = permalink.replace(disqus_blogger_homepage_url, disqus_blogger_canonical_homepage_url);
             }
         }

         tempanchor = document.createElement('a');
         tempanchor.className = 'comment-link disqus-blogger-comment-link';
         tempanchor.href = permalink + '#disqus_thread';
         for (var j=0; Math.min(j, changelist.length) != (changelist.length); j++) {
             changelist[j].innerHTML = '';
             changelist[j].appendChild(tempanchor);
             changelist[j].style.visibility = 'visible';
         }
         //if no comment-link elements were found, append to timestamp
         if (changelist.length === 0) {
             for(var h = 0; Math.min(h, spans.length) != (spans.length); h++) {
                 if (hasClass(spans[h], 'post-timestamp') || hasClass(spans[h], 'disqus-blogger-permalink'))
                     spans[h].appendChild(tempanchor);
             }
         }
     };
     var blog_block = get_blog_block();
     if (!!blog_block) {
         var posts = blog_block.getElementsByTagName('div');
         for(var i = 0; Math.min(i, posts.length) != (posts.length); i++) {
             if (hasClass(posts[i], "hentry") || hasClass(posts[i], "post") || hasClass(posts[i], 'disqus-blogger-post'))
                 change_links(posts[i]);
         }
         (function () {
              var s = document.createElement('script'); s.async = true;
              s.src = '//'+disqus_shortname+'.disqus.com/count.js';
              (document.getElementsByTagName('HEAD')[0] || document.body).appendChild(s);
          }());
     }
 })();
