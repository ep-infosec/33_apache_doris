-- SELECT body, count() FROM github_events WHERE body != "" AND length(body) < 100 GROUP BY body ORDER BY count() DESC LIMIT 50
