///|/ Copyright (c) Prusa Research 2020 - 2022 Tomáš Mészáros @tamasmeszaros, David Kocík @kocikdav
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#ifndef SLAIMPORTJOB_HPP
#define SLAIMPORTJOB_HPP

#include "Job.hpp"

namespace Slic3r { namespace GUI {

class SLAImportJobView
{
public:
    enum Sel { modelAndProfile, profileOnly, modelOnly };

    virtual ~SLAImportJobView() = default;

    virtual Sel         get_selection() const          = 0;
    virtual Vec2i       get_marchsq_windowsize() const = 0;
    virtual std::string get_path() const               = 0;
};

class Plater;

class SLAImportJob : public Job {
    class priv;

    std::unique_ptr<priv> p;
    using Sel = SLAImportJobView::Sel;

public:
    void prepare();
    void process(Ctl &ctl) override;
    void finalize(bool canceled, std::exception_ptr &) override;

    SLAImportJob(const SLAImportJobView *);
    ~SLAImportJob();

    void reset();
};

}}     // namespace Slic3r::GUI

#endif // SLAIMPORTJOB_HPP
