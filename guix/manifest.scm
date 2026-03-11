(use-modules (guix packages)
             (guix git-download)
             (guix utils)
             (gnu packages)) ; Traz a função specification->package

(define simgrid-v4
  (package
    (inherit (specification->package "simgrid"))
    (version "4.1")
    (source (origin
              (method git-fetch)
              (uri (git-reference
                    (url "https://framagit.org/simgrid/simgrid.git")
                    (commit "v4.1")))
              (file-name (git-file-name "simgrid" version))
              (sha256
               (base32
                "178nli9kzabahpvplj10da43yq693kfc39kf1nfkiifbzxd624h7"))))
    (arguments
     (substitute-keyword-arguments (package-arguments (specification->package "simgrid"))
       ((#:tests? _ #f) #f)
       ((#:configure-flags flags ''())
        `(cons "-Denable_documentation=OFF" ,flags))))))

(packages->manifest
 (list simgrid-v4
       (specification->package "openmpi") 
       (specification->package "ucx") 
       (specification->package "cmake")
       (specification->package "pkg-config")
       (specification->package "boost")
       (specification->package "make")
       (specification->package "gcc-toolchain")))
