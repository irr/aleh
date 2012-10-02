{
    'targets': [
        {
            'target_name': 'aleh',
            'type': 'executable',
            'sources': [
                'aleh.cc'
            ],
            'include_dirs': [
                '/usr/local/include/hiredis'
            ],
            'conditions': [
                [
                    'OS=="linux"',
                    {
                        'cflags': [
                            '-g',
                            '-std=c++11'
                        ],
                        'link_settings': {
                            'libraries': [
                                '-L/usr/local/lib',
                                '-levent',
                                '-lhiredis',
                                '-lpthread',
                                '-lrt'
                            ]
                        },
                        
                    }
                ],
                
            ]
        },
        
    ],   
}